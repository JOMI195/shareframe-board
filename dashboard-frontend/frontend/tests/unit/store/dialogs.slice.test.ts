import { describe, expect, it } from 'vitest';
import reducer, * as dialogs from '@/store/dialogs/dialogs.Slice';

const initial = () => reducer(undefined, { type: '@@tests/INIT' });

describe('dialogs slice', () => {
  it.each([
    ['shutdown', dialogs.openShutdownDialog, dialogs.closeShutdownDialog, (s: ReturnType<typeof initial>) => s.general.shutdown.open],
    ['restart', dialogs.openRestartDialog, dialogs.closeRestartDialog, (s: ReturnType<typeof initial>) => s.general.restart.open],
    ['change password', dialogs.openGeneralChangePasswordDialog, dialogs.closeGeneralChangePasswordDialog, (s: ReturnType<typeof initial>) => s.general.changePassword.open],
    ['confirm update', dialogs.openUpdatesConfirmUpdateDialog, dialogs.closeUpdatesConfirmUpdateDialog, (s: ReturnType<typeof initial>) => s.updates.confirmUpdate.open],
    ['add network', dialogs.openNetworkAddNetworkDialog, dialogs.closeNetworkAddNetworkDialog, (s: ReturnType<typeof initial>) => s.network.addNetwork.open],
    ['AP password', dialogs.openNetworkChangeApPasswordDialog, dialogs.closeNetworkChangeApPasswordDialog, (s: ReturnType<typeof initial>) => s.network.changeApPassword.open],
  ])('opens and closes the %s dialog', (_name, open, close, isOpen) => {
    const opened = reducer(initial(), open());
    expect(isOpen(opened)).toBe(true);

    expect(isOpen(reducer(opened, close()))).toBe(false);
  });

  it('remembers which network to forget and clears it on close', () => {
    const opened = reducer(initial(), dialogs.openNetworkForgetNetworkDialog('HomeNet'));
    expect(opened.network.forgetNetwork).toEqual({ open: true, ssid: 'HomeNet' });

    expect(reducer(opened, dialogs.closeNetworkForgetNetworkDialog()).network.forgetNetwork).toEqual({ open: false, ssid: '' });
  });

  it('seeds the rename field with the current name', () => {
    let state = reducer(initial(), dialogs.openNetworkRenameNetworkDialog('HomeNet'));
    expect(state.network.renameNetwork).toEqual({ open: true, ssid: 'HomeNet', newName: 'HomeNet' });

    state = reducer(state, dialogs.updateNetworkRenameNetworkName('Office'));
    expect(state.network.renameNetwork.newName).toBe('Office');

    state = reducer(state, dialogs.closeNetworkRenameNetworkDialog());
    expect(state.network.renameNetwork).toEqual({ open: false, ssid: '', newName: '' });
  });
});
