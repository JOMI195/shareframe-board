import { afterEach, describe, expect, it, vi } from 'vitest';
import { boardErrorMessage, OFFLINE_MESSAGE, UNREACHABLE_MESSAGE, unreachableMessage } from '@/common/utils/errors/boardErrors';

describe('board error messages', () => {
  afterEach(() => vi.restoreAllMocks());

  it('translates a known board message', () => {
    expect(boardErrorMessage('Anmeldung fehlgeschlagen', { status: 401 }, 'OTP invalid')).toBe('Der Einmal-Code stimmt nicht oder ist abgelaufen.');
  });

  it('falls back to the status for unknown messages', () => {
    expect(boardErrorMessage('Konnte nicht gespeichert werden', { status: 500 }, 'IPC error')).toBe('Auf dem Bilderrahmen ist ein Fehler aufgetreten.');
    expect(boardErrorMessage('Konnte nicht gespeichert werden', { status: 429 })).toBe('Zu viele Versuche. Bitte warte kurz.');
  });

  it('keeps the caller text when neither tells more', () => {
    expect(boardErrorMessage('Konnte nicht gespeichert werden', { status: 400 }, 'id is required')).toBe('Konnte nicht gespeichert werden');
    expect(boardErrorMessage('Konnte nicht gespeichert werden')).toBe('Konnte nicht gespeichert werden');
  });

  it('tells offline apart from an unreachable board', () => {
    vi.spyOn(navigator, 'onLine', 'get').mockReturnValue(false);
    expect(unreachableMessage()).toBe(OFFLINE_MESSAGE);

    vi.spyOn(navigator, 'onLine', 'get').mockReturnValue(true);
    expect(unreachableMessage()).toBe(UNREACHABLE_MESSAGE);
  });
});
