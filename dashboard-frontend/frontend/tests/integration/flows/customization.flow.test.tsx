import { describe, expect, it } from 'vitest';
import { screen, waitFor } from '@testing-library/react';
import * as api from '@/assets/endpoints/api/frame';
import { renderRoute } from '@tests/helpers/renderRoute';
import { signedInState } from '@tests/helpers/preloadedState';
import { failsWith, ok, server, setBoard, spyOnRequests } from '@tests/mocks';

const senderSwitch = () => screen.getByLabelText('Name des Absenders anzeigen');

const openCustomization = async () => {
  const view = renderRoute('/customization/', { preloadedState: signedInState() });
  await waitFor(() => expect(senderSwitch()).toBeEnabled());
  return view;
};

describe('customization', () => {
  it('shows the stored choice and turns the sender name off on the board', async () => {
    setBoard({ showSender: true });
    const save = spyOnRequests('post', api.getDisplayCustomizationUrl(), () => ok({ show_sender: false }));
    server.use(save.handler);
    const { user } = await openCustomization();
    expect(senderSwitch()).toBeChecked();

    await user.click(senderSwitch());

    await waitFor(() => expect(save.calls).toHaveLength(1));
    expect(await save.calls[0].json()).toEqual({ show_sender: false });
    expect(await screen.findByText('Der Name des Absenders wird ab dem nächsten Bildwechsel ausgeblendet')).toBeInTheDocument();
    expect(senderSwitch()).not.toBeChecked();
  });

  it('a failed save leaves the switch off and says so', async () => {
    server.use(...failsWith('post', api.getDisplayCustomizationUrl()));
    const { user } = await openCustomization();
    expect(senderSwitch()).not.toBeChecked();

    await user.click(senderSwitch());

    expect(await screen.findByText('Speichern der Anpassung fehlgeschlagen')).toBeInTheDocument();
    expect(senderSwitch()).not.toBeChecked();
  });
});
