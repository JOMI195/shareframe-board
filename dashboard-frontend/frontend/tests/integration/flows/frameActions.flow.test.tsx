import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';
import { screen, within } from '@testing-library/react';
import { HttpResponse } from 'msw';
import * as api from '@/assets/endpoints/api/frame';
import { card } from '@tests/helpers/card';
import { renderRoute } from '@tests/helpers/renderRoute';
import { signedInState } from '@tests/helpers/preloadedState';
import { advance, INITIAL_LOAD_LOCK_MS, useBoardClock } from '@tests/helpers/time';
import { failsWith, server, setBoard, spyOnRequests, withSlideshowStatus } from '@tests/mocks';

const openHome = async () => {
  const view = renderRoute('/', { preloadedState: signedInState() });
  await screen.findByRole('heading', { level: 2, name: 'Diashow' });
  await advance(INITIAL_LOAD_LOCK_MS);
  return view;
};

describe('frame actions', () => {
  beforeEach(() => {
    useBoardClock();
  });

  afterEach(() => {
    vi.useRealTimers();
  });

  it('shows the slideshow status and the countdown to the next image', async () => {
    server.use(...withSlideshowStatus({ seconds_until_next: 125, image_count: 1 }));
    await openHome();

    expect(within(card('Diashow')).getByText('Wird ausgeführt')).toBeInTheDocument();
    const next = card('Nächster Fotowechsel');
    expect(within(next).getByText('1 Foto')).toBeInTheDocument();
    expect(within(next).getByText(/^2:0\d$/)).toBeInTheDocument();
  });

  it('shows the picture on the panel and drops it once the panel is cleared', async () => {
    const file = spyOnRequests('get', api.getImageFileUrl(0).split('?')[0],
      () => new HttpResponse(new Uint8Array([0xff, 0xd8]), { headers: { 'Content-Type': 'image/jpeg' } }));
    server.use(file.handler, ...withSlideshowStatus({
      now_showing: { seq: 4, state: 'image', image_id: 12, sender: 'bob' },
    }));
    await openHome();

    const shown = card('Aktuell angezeigt');
    expect(within(shown).getByText('Foto von bob')).toBeInTheDocument();
    expect(await within(shown).findByRole('img', { name: 'Aktuell angezeigtes Foto' })).toBeInTheDocument();
    expect(new URL(file.calls[0].url).searchParams.get('id')).toBe('12');

    server.use(...withSlideshowStatus({ now_showing: { seq: 5, state: 'cleared', image_id: 0, sender: '' } }));
    await advance(5_000);

    expect(await within(shown).findByText('Display geleert')).toBeInTheDocument();
    expect(within(shown).queryByRole('img')).not.toBeInTheDocument();
  });

  it('locks the actions until the initial status is known', async () => {
    renderRoute('/', { preloadedState: signedInState() });

    expect(
      await screen.findByText('Einen Moment, der Status der Diashow wird abgefragt.'),
    ).toBeInTheDocument();
    expect(screen.getByRole('button', { name: 'Stoppen' })).toBeDisabled();

    await advance(INITIAL_LOAD_LOCK_MS);

    expect(screen.getByRole('button', { name: 'Stoppen' })).toBeEnabled();
  });

  it('stops the slideshow, clears the panel and locks the actions', async () => {
    const clear = spyOnRequests('post', api.getClearDisplayUrl());
    server.use(clear.handler);
    const { user } = await openHome();

    await user.click(screen.getByRole('button', { name: 'Stoppen' }));

    expect(await screen.findByText('Diashow gestoppt')).toBeInTheDocument();
    expect(clear.calls).toHaveLength(1);
    expect(screen.getByText(/nächste Aktion erst wieder in 0[23]:\d\dmin möglich/)).toBeInTheDocument();

    await advance(5_000);
    expect(within(card('Diashow')).getByText('Gestoppt')).toBeInTheDocument();
    expect(screen.getByRole('button', { name: 'Starten' })).toBeDisabled();
  });

  it('starts a stopped slideshow', async () => {
    setBoard({ slideshowActive: false });
    const { user } = await openHome();

    await user.click(screen.getByRole('button', { name: 'Starten' }));

    expect(await screen.findByText('Diashow gestartet')).toBeInTheDocument();
  });

  it('skips the current image', async () => {
    const skip = spyOnRequests('post', api.getSkipSlideshowImageUrl());
    server.use(skip.handler);
    const { user } = await openHome();

    await user.click(screen.getByRole('button', { name: 'Foto überspringen' }));

    expect(await screen.findByText('Foto übersprungen')).toBeInTheDocument();
    expect(skip.calls).toHaveLength(1);
  });

  it('only clears the panel while the slideshow is stopped', async () => {
    setBoard({ slideshowActive: false });
    const { user } = await openHome();

    expect(screen.getByRole('button', { name: 'Foto überspringen' })).toBeDisabled();
    await user.click(screen.getByRole('button', { name: 'Bildschirm leeren' }));

    expect(await screen.findByText('Bildschirm geleert')).toBeInTheDocument();
  });

  it('reports a failed skip', async () => {
    server.use(...failsWith('post', api.getSkipSlideshowImageUrl()));
    const { user } = await openHome();

    await user.click(screen.getByRole('button', { name: 'Foto überspringen' }));

    expect(await screen.findByText('Foto konnte nicht übersprungen werden')).toBeInTheDocument();
    expect(screen.getByRole('button', { name: 'Foto überspringen' })).toBeEnabled();
  });

  it('locks the actions while the panel refreshes', async () => {
    server.use(...withSlideshowStatus({ refreshing: true }));
    await openHome();

    expect(screen.getByText('Wird aktualisiert…')).toBeInTheDocument();
    expect(screen.getByRole('button', { name: 'Foto überspringen' })).toBeDisabled();
    expect(screen.getByRole('button', { name: 'Stoppen' })).toBeDisabled();
  });

  it('reports a skip rejected during a refresh', async () => {
    server.use(...failsWith('post', api.getSkipSlideshowImageUrl(), 409, 'Display is refreshing'));
    const { user } = await openHome();

    await user.click(screen.getByRole('button', { name: 'Foto überspringen' }));

    expect(await screen.findByText('Das Display wird gerade aktualisiert. Versuche es gleich noch einmal.')).toBeInTheDocument();
  });

  it('saves the interval in seconds', async () => {
    const interval = spyOnRequests('post', api.getSlideshowIntervalUrl());
    server.use(interval.handler);
    const { user } = await openHome();

    const input = within(card('Wechselintervall')).getByLabelText('Intervall');
    await user.clear(input);
    await user.type(input, '30');
    await user.click(screen.getByRole('button', { name: 'Intervall speichern' }));

    expect(await screen.findByText('Intervall auf 30 Minuten gesetzt')).toBeInTheDocument();
    expect(await interval.calls[0].json()).toEqual({ interval_seconds: 1800 });
  });

  it('rejects an interval below five minutes', async () => {
    const { user } = await openHome();

    const input = within(card('Wechselintervall')).getByLabelText('Intervall');
    await user.clear(input);
    await user.type(input, '4');

    expect(screen.getByRole('button', { name: 'Intervall speichern' })).toBeDisabled();
  });

  it('enables night mode with the configured window', async () => {
    const night = spyOnRequests('post', api.getSlideshowNightModeUrl());
    server.use(night.handler);
    const { user } = await openHome();

    await user.click(screen.getByLabelText('Nachtmodus aktivieren'));
    await user.click(screen.getByRole('button', { name: 'Nachtmodus speichern' }));

    expect(await screen.findByText('Nachtmodus von 2:00 bis 5:00 Uhr mit 60 Minuten Intervall gespeichert')).toBeInTheDocument();
    expect(await night.calls[0].json()).toEqual({ enabled: true, start_hour: 2, end_hour: 5, interval_seconds: 3600, style: 'slower' });
  });

  it.each([
    ['Bildschirm pausieren', 'pause', 'ausgeschaltetem Bildschirm'],
    ['Aktuelles Foto behalten', 'hold', 'festgehaltenem Foto'],
  ])('can %s at night instead of slowing down', async (option, style, suffix) => {
    const night = spyOnRequests('post', api.getSlideshowNightModeUrl());
    server.use(night.handler);
    const { user } = await openHome();

    await user.click(screen.getByLabelText('Nachtmodus aktivieren'));
    await user.click(screen.getByRole('combobox', { name: 'Verhalten in der Nacht' }));
    await user.click(await screen.findByRole('option', { name: option }));
    expect(within(card('Nachtmodus')).getByLabelText('Intervall in der Nacht (Minuten)')).toBeDisabled();
    await user.click(screen.getByRole('button', { name: 'Nachtmodus speichern' }));

    expect(await screen.findByText(`Nachtmodus von 2:00 bis 5:00 Uhr mit ${suffix} gespeichert`)).toBeInTheDocument();
    expect(await night.calls[0].json()).toMatchObject({ enabled: true, style });
  });

  it('saving night mode does not lock the display actions', async () => {
    const { user } = await openHome();

    await user.click(screen.getByLabelText('Nachtmodus aktivieren'));
    await user.click(screen.getByRole('button', { name: 'Nachtmodus speichern' }));

    expect(await screen.findByText('Nachtmodus von 2:00 bis 5:00 Uhr mit 60 Minuten Intervall gespeichert')).toBeInTheDocument();
    expect(screen.queryByText(/nächste Aktion erst wieder/)).not.toBeInTheDocument();
    expect(screen.getByRole('button', { name: 'Foto überspringen' })).toBeEnabled();
  });

  it('keeps night mode editable while the display actions are locked', async () => {
    const { user } = await openHome();

    await user.click(screen.getByRole('button', { name: 'Foto überspringen' }));
    expect(await screen.findByText(/nächste Aktion erst wieder/)).toBeInTheDocument();

    await user.click(screen.getByLabelText('Nachtmodus aktivieren'));
    await user.click(screen.getByRole('button', { name: 'Nachtmodus speichern' }));

    expect(await screen.findByText('Nachtmodus von 2:00 bis 5:00 Uhr mit 60 Minuten Intervall gespeichert')).toBeInTheDocument();
  });

  it('keeps night mode editable during the initial status lock', async () => {
    renderRoute('/', { preloadedState: signedInState() });

    expect(
      await screen.findByText('Einen Moment, der Status der Diashow wird abgefragt.'),
    ).toBeInTheDocument();
    await advance(5_000);

    expect(screen.getByRole('button', { name: 'Stoppen' })).toBeDisabled();
    expect(screen.getByLabelText('Nachtmodus aktivieren')).toBeEnabled();
  });

  it('marks night mode while it is active', async () => {
    server.use(
      ...withSlideshowStatus({
        night_mode: { enabled: true, start_hour: 22, end_hour: 6, interval_seconds: 7200, style: 'slower', active_now: true },
      }),
    );
    await openHome();

    expect(within(card('Nächster Fotowechsel')).getByText('Nachtmodus')).toBeInTheDocument();
    expect(screen.getByLabelText('Nachtmodus aktivieren')).toBeChecked();
    expect(within(card('Nachtmodus')).getByLabelText('Intervall in der Nacht (Minuten)')).toHaveValue(120);
  });
});
