import { describe, expect, it, vi } from 'vitest';
import { fireEvent, screen, waitFor, within } from '@testing-library/react';
import * as api from '@/assets/endpoints/api/frame';
import { renderRoute } from '@tests/helpers/renderRoute';
import { signedInState } from '@tests/helpers/preloadedState';
import { makeSlideshowImages } from '@tests/fixtures';
import { failsWith, server, setBoard, spyOnRequests } from '@tests/mocks';

const openSlideshowImages = async () => {
  const view = renderRoute('/slideshow/', { preloadedState: signedInState() });
  await screen.findByRole('listitem', { name: /Bild von alice/ });
  return view;
};

// While sorting, dnd-kit turns the tiles into buttons, so they are found by label.
const senders = () => screen.getAllByLabelText(/^Bild von \w+, Position/)
  .map((tile) => tile.getAttribute('aria-label')!.match(/Bild von (\w+)/)![1]);

describe('slideshow images', () => {
  it('lists the pictures in play order and marks the one on the panel', async () => {
    setBoard({ slideshowImages: makeSlideshowImages({ current_image_id: 12 }) });
    await openSlideshowImages();

    expect(senders()).toEqual(['alice', 'bob', 'carol']);
    const shown = screen.getByRole('listitem', { name: /Bild von bob/ });
    expect(within(shown).getByText('Wird angezeigt')).toBeInTheDocument();
    expect(within(screen.getByRole('listitem', { name: /Bild von alice/ })).queryByText('Wird angezeigt')).not.toBeInTheDocument();
    expect(screen.getByText('3 Bilder auf dem Bilderrahmen')).toBeInTheDocument();
  });

  it('switching to newest first applies the order the board reports back', async () => {
    const { user } = await openSlideshowImages();

    await user.click(screen.getByRole('button', { name: 'Neueste zuerst' }));

    expect(await screen.findByText('Neueste zuerst ab dem nächsten Bild')).toBeInTheDocument();
    await waitFor(() => expect(senders()).toEqual(['carol', 'bob', 'alice']));
    expect(screen.getByRole('button', { name: 'Neueste zuerst' })).toHaveAttribute('aria-pressed', 'true');
  });

  it('saves a hand-picked order with every picture in it', async () => {
    const order = spyOnRequests('post', api.getSlideshowOrderUrl());
    server.use(order.handler);
    const { user } = await openSlideshowImages();

    await user.click(screen.getByRole('button', { name: 'Eigene Reihenfolge' }));
    await user.click(screen.getByRole('button', { name: 'Bild von carol nach vorne verschieben' }));
    await user.click(screen.getByRole('button', { name: 'Bild von carol nach vorne verschieben' }));
    expect(senders()).toEqual(['carol', 'alice', 'bob']);
    await user.click(screen.getByRole('button', { name: 'Reihenfolge speichern' }));

    await waitFor(() => expect(order.calls).toHaveLength(1));
    expect(await order.calls[0].json()).toEqual({ mode: 'custom', custom_order: [13, 11, 12] });
    await waitFor(() => expect(screen.queryByRole('button', { name: 'Reihenfolge speichern' })).not.toBeInTheDocument());
  });

  it('discarding a draft restores the order and sends nothing', async () => {
    const order = spyOnRequests('post', api.getSlideshowOrderUrl());
    server.use(order.handler);
    const { user } = await openSlideshowImages();

    await user.click(screen.getByRole('button', { name: 'Eigene Reihenfolge' }));
    await user.click(screen.getByRole('button', { name: 'Bild von alice nach hinten verschieben' }));
    expect(senders()).toEqual(['bob', 'alice', 'carol']);
    await user.click(screen.getByRole('button', { name: 'Verwerfen' }));

    expect(senders()).toEqual(['alice', 'bob', 'carol']);
    expect(screen.getByRole('button', { name: 'Chronologisch' })).toHaveAttribute('aria-pressed', 'true');
    expect(order.calls).toHaveLength(0);
  });

  it('keeps the draft when the board rejects the order', async () => {
    server.use(...failsWith('post', api.getSlideshowOrderUrl(), 400, 'custom_order must be a list of picture ids'));
    const { user } = await openSlideshowImages();

    await user.click(screen.getByRole('button', { name: 'Eigene Reihenfolge' }));
    await user.click(screen.getByRole('button', { name: 'Bild von alice nach hinten verschieben' }));
    await user.click(screen.getByRole('button', { name: 'Reihenfolge speichern' }));

    expect(await screen.findByText('custom_order must be a list of picture ids')).toBeInTheDocument();
    expect(senders()).toEqual(['bob', 'alice', 'carol']);
    expect(screen.getByRole('button', { name: 'Reihenfolge speichern' })).toBeInTheDocument();
  });

  it('opens a picture with its details', async () => {
    const { user } = await openSlideshowImages();

    await user.click(screen.getByRole('listitem', { name: /Bild von bob/ }));

    const dialog = await screen.findByRole('dialog');
    expect(within(dialog).getByText('800 × 480 px')).toBeInTheDocument();
    expect(within(dialog).getByText('345.68 KB')).toBeInTheDocument();
    expect(within(dialog).getByText('Position 2 von 3')).toBeInTheDocument();
    expect(within(dialog).getByText('09.10.2025, 09:03:20')).toBeInTheDocument();
    expect(await within(dialog).findByRole('img', { name: 'Bild von bob' })).toHaveAttribute('src', expect.stringMatching(/^blob:/));
  });

  it('puts a picture on the panel from its preview', async () => {
    const show = spyOnRequests('post', api.getShowSlideshowImageUrl());
    server.use(show.handler);
    const { user } = await openSlideshowImages();

    await user.click(screen.getByRole('listitem', { name: /Bild von carol/ }));
    const button = await within(await screen.findByRole('dialog')).findByRole('button', { name: 'Jetzt anzeigen' });
    await waitFor(() => expect(button).toBeEnabled());
    await user.click(button);

    await waitFor(() => expect(show.calls).toHaveLength(1));
    expect(await show.calls[0].json()).toEqual({ id: 13 });
    expect(await screen.findByText('Bild wird auf dem Bilderrahmen angezeigt')).toBeInTheDocument();
  });

  it('cannot show the picture that is already on the panel', async () => {
    setBoard({ slideshowImages: makeSlideshowImages({ current_image_id: 12 }) });
    const { user } = await openSlideshowImages();

    await user.click(screen.getByRole('listitem', { name: /Bild von bob/ }));

    expect(await within(await screen.findByRole('dialog')).findByRole('button', { name: 'Jetzt anzeigen' })).toBeDisabled();
  });

  it('cannot show a picture while the slideshow is paused', async () => {
    setBoard({ slideshowActive: false });
    const { user } = await openSlideshowImages();

    await user.click(screen.getByRole('listitem', { name: /Bild von carol/ }));

    expect(await within(await screen.findByRole('dialog')).findByRole('button', { name: 'Jetzt anzeigen' })).toBeDisabled();
  });

  it('says to retry when the panel is busy refreshing', async () => {
    server.use(...failsWith('post', api.getShowSlideshowImageUrl(), 409, 'Display is refreshing'));
    const { user } = await openSlideshowImages();

    await user.click(screen.getByRole('listitem', { name: /Bild von carol/ }));
    const button = await within(await screen.findByRole('dialog')).findByRole('button', { name: 'Jetzt anzeigen' });
    await waitFor(() => expect(button).toBeEnabled());
    await user.click(button);

    expect(await screen.findByText('Display wird gerade aktualisiert, bitte gleich erneut versuchen')).toBeInTheDocument();
  });

  it('downloads a picture from its preview', async () => {
    const click = vi.spyOn(HTMLAnchorElement.prototype, 'click').mockImplementation(() => undefined);
    const { user } = await openSlideshowImages();

    await user.click(screen.getByRole('listitem', { name: /Bild von carol/ }));
    await within(await screen.findByRole('dialog')).findByRole('button', { name: 'Jetzt anzeigen' });
    await user.hover(screen.getByRole('button', { name: 'Aktionen' }));
    const label = await screen.findByText('Foto herunterladen');
    // In jsdom the open speed dial's actions still report pointer-events: none.
    fireEvent.click(within(label.closest('.MuiSpeedDialAction-staticTooltip') as HTMLElement).getByRole('menuitem'));

    await waitFor(() => expect(click).toHaveBeenCalledTimes(1));
    expect((click.mock.instances[0] as unknown as HTMLAnchorElement).download).toBe('carol-13.jpg');
    click.mockRestore();
  });

  it('a saved custom order can be edited again starting from itself', async () => {
    const [alice, bob, carol] = makeSlideshowImages().images;
    setBoard({ slideshowImages: makeSlideshowImages({ mode: 'custom', custom_order: [13, 11, 12], images: [carol, alice, bob] }) });
    const order = spyOnRequests('post', api.getSlideshowOrderUrl());
    server.use(order.handler);
    const { user } = await openSlideshowImages();

    await user.click(screen.getByRole('button', { name: 'Reihenfolge bearbeiten' }));
    await user.click(screen.getByRole('button', { name: 'Bild von bob nach vorne verschieben' }));
    await user.click(screen.getByRole('button', { name: 'Reihenfolge speichern' }));

    await waitFor(() => expect(order.calls).toHaveLength(1));
    expect(await order.calls[0].json()).toEqual({ mode: 'custom', custom_order: [13, 12, 11] });
  });

  it('explains a failed load instead of claiming the frame is empty', async () => {
    server.use(...failsWith('get', api.getSlideshowImagesUrl(), 500, 'Service unavailable'));
    renderRoute('/slideshow/', { preloadedState: signedInState() });

    expect(await screen.findByText('Die Bilder konnten nicht geladen werden.')).toBeInTheDocument();
    expect(screen.queryByText(/Keine Bilder auf dem Bilderrahmen/)).not.toBeInTheDocument();
  });

  it('says so when the frame holds no pictures', async () => {
    setBoard({ slideshowImages: makeSlideshowImages({ images: [] }) });
    renderRoute('/slideshow/', { preloadedState: signedInState() });

    expect(await screen.findByText('Keine Bilder auf dem Bilderrahmen. Es werden die Standardbilder angezeigt.')).toBeInTheDocument();
  });
});
