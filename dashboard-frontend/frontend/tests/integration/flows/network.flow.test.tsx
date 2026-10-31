import { describe, expect, it } from 'vitest';
import { screen, waitFor, within } from '@testing-library/react';
import * as api from '@/assets/endpoints/api/frame';
import { card } from '@tests/helpers/card';
import { renderRoute } from '@tests/helpers/renderRoute';
import { signedInState } from '@tests/helpers/preloadedState';
import { failsWith, server, spyOnRequests, withConnectionMode, withNetworks } from '@tests/mocks';

const openNetwork = async (current = 'HomeNet', saved = ['HomeNet', 'Office']) => {
  server.use(...withNetworks(current, saved), ...withConnectionMode({ ap_password: 'hotspot123' }));
  const view = renderRoute('/network/', { preloadedState: signedInState() });
  await within(await screen.findByRole('list')).findByText(saved[0]);
  return view;
};

describe('network', () => {
  it('lists the current and the saved networks', async () => {
    await openNetwork();

    const overview = card('WLAN');
    expect(within(overview).getAllByText('HomeNet')).toHaveLength(2);
    expect(within(overview).getByText('Office')).toBeInTheDocument();
  });

  it('says so when no network is saved', async () => {
    server.use(...withNetworks('Preset', []));
    renderRoute('/network/', { preloadedState: signedInState() });

    expect(await screen.findByText(/Noch keine WLANs gespeichert/)).toBeInTheDocument();
  });

  it('adds a network and reloads the list', async () => {
    const connect = spyOnRequests('post', api.getConnectionConnectUrl());
    server.use(connect.handler);
    const { user } = await openNetwork();

    await user.click(screen.getByRole('button', { name: 'WLAN hinzufügen' }));
    const dialog = await screen.findByRole('dialog');
    await user.type(within(dialog).getByLabelText(/Netzwerkname/), 'Garden');
    await user.type(within(dialog).getByLabelText(/^Passwort$/), 'gardenpass');
    server.use(...withNetworks('HomeNet', ['HomeNet', 'Office', 'Garden']));
    await user.click(within(dialog).getByRole('button', { name: 'Hinzufügen' }));

    expect(await screen.findByText('WLAN hinzugefügt')).toBeInTheDocument();
    expect(await connect.calls[0].json()).toEqual({ ssid: 'Garden', password: 'gardenpass' });
    await waitFor(() => expect(screen.queryByRole('dialog')).not.toBeInTheDocument());
    expect(await within(card('WLAN')).findByText('Garden')).toBeInTheDocument();
  });

  it('keeps the dialog open when adding fails', async () => {
    server.use(...failsWith('post', api.getConnectionConnectUrl()));
    const { user } = await openNetwork();

    await user.click(screen.getByRole('button', { name: 'WLAN hinzufügen' }));
    const dialog = await screen.findByRole('dialog');
    await user.type(within(dialog).getByLabelText(/Netzwerkname/), 'Garden');
    await user.type(within(dialog).getByLabelText(/^Passwort$/), 'gardenpass');
    await user.click(within(dialog).getByRole('button', { name: 'Hinzufügen' }));

    expect(await screen.findByText('Auf dem Bilderrahmen ist ein Fehler aufgetreten.')).toBeInTheDocument();
    expect(screen.getByRole('dialog')).toBeInTheDocument();
  });

  it('requires SSID and password', async () => {
    const connect = spyOnRequests('post', api.getConnectionConnectUrl());
    server.use(connect.handler);
    const { user } = await openNetwork();

    await user.click(screen.getByRole('button', { name: 'WLAN hinzufügen' }));
    const dialog = await screen.findByRole('dialog');
    await user.type(within(dialog).getByLabelText(/Netzwerkname/), 'Garden');
    await user.click(within(dialog).getByRole('button', { name: 'Hinzufügen' }));

    expect(await screen.findByText('Gib Netzwerkname und Passwort ein')).toBeInTheDocument();
    expect(connect.calls).toHaveLength(0);
  });

  it('forgets a saved network after confirmation', async () => {
    const forget = spyOnRequests('post', api.getConnectionForgetUrl());
    server.use(forget.handler);
    const { user } = await openNetwork();

    const office = within(card('WLAN')).getByText('Office').closest('li') as HTMLElement;
    await user.click(within(office).getByRole('button', { name: '„Office“ entfernen' }));
    await user.click(within(await screen.findByRole('dialog')).getByRole('button', { name: 'Entfernen' }));

    expect(await screen.findByText('WLAN entfernt')).toBeInTheDocument();
    expect(await forget.calls[0].json()).toEqual({ ssid: 'Office' });
  });

  it('shows the hotspot password only on request', async () => {
    const { user } = await openNetwork();

    const ap = card('Hotspot');
    expect(await within(ap).findByText('shareframe-board')).toBeInTheDocument();
    expect(within(ap).queryByText('hotspot123')).not.toBeInTheDocument();

    await user.click(within(ap).getByRole('button', { name: 'Hotspot-Passwort anzeigen' }));

    expect(within(ap).getByText('hotspot123')).toBeInTheDocument();
  });

  it('changes the hotspot password once both entries match', async () => {
    const apPassword = spyOnRequests('post', api.getApPasswordUrl());
    server.use(apPassword.handler);
    const { user } = await openNetwork();

    await user.click(screen.getByRole('button', { name: 'Hotspot-Passwort ändern' }));
    const dialog = await screen.findByRole('dialog');
    const confirm = within(dialog).getByRole('button', { name: 'Passwort ändern' });

    await user.type(within(dialog).getByLabelText(/^Neues Hotspot-Passwort$/), 'newhotspot');
    await user.type(within(dialog).getByLabelText(/Hotspot-Passwort bestätigen/), 'newhotspoX');
    expect(within(dialog).getByText('Passwörter stimmen nicht überein')).toBeInTheDocument();
    expect(confirm).toBeDisabled();

    await user.clear(within(dialog).getByLabelText(/Hotspot-Passwort bestätigen/));
    await user.type(within(dialog).getByLabelText(/Hotspot-Passwort bestätigen/), 'newhotspot');
    await user.click(confirm);

    expect(await screen.findByText('Hotspot-Passwort geändert')).toBeInTheDocument();
    expect(await apPassword.calls[0].json()).toEqual({ password: 'newhotspot' });
  });

  it('rejects a hotspot password shorter than eight characters', async () => {
    const { user } = await openNetwork();

    await user.click(screen.getByRole('button', { name: 'Hotspot-Passwort ändern' }));
    const dialog = await screen.findByRole('dialog');
    await user.type(within(dialog).getByLabelText(/^Neues Hotspot-Passwort$/), 'short');
    await user.type(within(dialog).getByLabelText(/Hotspot-Passwort bestätigen/), 'short');

    expect(within(dialog).getByRole('button', { name: 'Passwort ändern' })).toBeDisabled();
  });
});
