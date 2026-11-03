import { describe, expect, it, vi } from 'vitest';
import { act, screen, waitFor } from '@testing-library/react';
import ConfirmDialog from '@/common/components/confirmDialog';
import { renderWithProviders } from '@tests/helpers/renderWithProviders';

const renderConfirm = (props: Partial<React.ComponentProps<typeof ConfirmDialog>> = {}) => {
  const onConfirm = vi.fn();
  const onCancel = vi.fn();
  const view = renderWithProviders(
    <ConfirmDialog open title="Wirklich?" confirmLabel="Löschen" onConfirm={onConfirm} onCancel={onCancel} {...props}>
      Das kann nicht rückgängig gemacht werden.
    </ConfirmDialog>,
  );
  return { ...view, onConfirm, onCancel };
};

describe('confirm dialog', () => {
  it('starts on the action, but on Abbrechen when it destroys something', () => {
    const { unmount } = renderConfirm();
    expect(screen.getByRole('button', { name: 'Löschen' })).toHaveFocus();
    unmount();

    renderConfirm({ destructive: true });
    expect(screen.getByRole('button', { name: 'Abbrechen' })).toHaveFocus();
  });

  it('keeps both buttons focusable but inert while busy', async () => {
    const { user, onConfirm, onCancel } = renderConfirm({ loading: true });

    const confirm = screen.getByRole('button', { name: /Löschen/ });
    expect(confirm).toHaveAttribute('aria-disabled', 'true');
    expect(confirm).not.toBeDisabled();

    await user.click(confirm);
    await user.click(screen.getByRole('button', { name: 'Abbrechen' }));
    expect(onConfirm).not.toHaveBeenCalled();
    expect(onCancel).not.toHaveBeenCalled();
  });

  it('closes on the back button instead of leaving the page', async () => {
    const { onCancel } = renderConfirm();

    await act(async () => window.history.back());

    await waitFor(() => expect(onCancel).toHaveBeenCalledTimes(1));
  });
});
