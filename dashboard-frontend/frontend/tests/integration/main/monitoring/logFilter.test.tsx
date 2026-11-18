import { describe, expect, it, vi } from 'vitest';
import { fireEvent, screen } from '@testing-library/react';
import LogFilter from '@/main/monitoring/logFilter';
import { renderWithProviders } from '@tests/helpers/renderWithProviders';

describe('log filter', () => {
  it("starts with the default range and line count in the board's timestamp format", () => {
    const onChange = vi.fn();
    renderWithProviders(<LogFilter onChange={onChange} />);
    expect(onChange).toHaveBeenLastCalledWith({
      since_timestamp: expect.stringMatching(/^\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}$/),
      lines: 1000,
    });
  });

  it('only a positive line count is sent; the field goes back to it when left', () => {
    const onChange = vi.fn();
    renderWithProviders(<LogFilter onChange={onChange} />);
    const lines = screen.getByLabelText('Anzahl Zeilen');

    fireEvent.change(lines, { target: { value: '250' } });
    expect(onChange).toHaveBeenLastCalledWith(expect.objectContaining({ lines: 250 }));

    fireEvent.change(lines, { target: { value: '-3' } });
    expect(onChange).toHaveBeenLastCalledWith(expect.objectContaining({ lines: 250 }));
    fireEvent.blur(lines);
    expect(lines).toHaveValue(250);
  });
});
