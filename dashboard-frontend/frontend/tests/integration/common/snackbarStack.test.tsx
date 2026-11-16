import { describe, expect, it } from 'vitest';
import { render, screen } from '@testing-library/react';
import { StackedSnackbar } from '@/common/components/snackbars/snackbarStack';
import { renderWithProviders } from '@tests/helpers/renderWithProviders';

const Two = ({ second = true }: { second?: boolean }) => (
  <>
    <StackedSnackbar open message="Erste" />
    <StackedSnackbar open={second} message="Zweite" />
  </>
);

const snackbarOf = (text: string) => screen.getByText(text).closest('.MuiSnackbar-root') as HTMLElement;

describe('StackedSnackbar', () => {
  it('puts snackbars that are open at once into one column, in order', async () => {
    renderWithProviders(<Two />);

    await screen.findByText('Erste');
    const first = snackbarOf('Erste');
    const second = snackbarOf('Zweite');

    expect(first.parentElement).toBe(second.parentElement);
    expect(first.compareDocumentPosition(second) & Node.DOCUMENT_POSITION_FOLLOWING).toBeTruthy();
    expect(first).toHaveStyle({ position: 'static' });
  });

  // Phones stretch each snackbar across the column, so content narrower than that must not hug the left edge.
  it('centers its content in the column', async () => {
    renderWithProviders(<Two second={false} />);

    await screen.findByText('Erste');
    expect(snackbarOf('Erste')).toHaveStyle({ justifyContent: 'center' });
  });

  it('leaves a closed snackbar out of the column', async () => {
    renderWithProviders(<Two second={false} />);

    await screen.findByText('Erste');
    expect(screen.queryByText('Zweite')).not.toBeInTheDocument();
    expect(snackbarOf('Erste').parentElement?.children).toHaveLength(1);
  });

  // Components rendered on their own, e.g. in isolated tests, still show their snackbar.
  it('falls back to a plain snackbar without the provider', () => {
    render(<StackedSnackbar open message="Allein" />);

    expect(screen.getByText('Allein')).toBeVisible();
  });
});
