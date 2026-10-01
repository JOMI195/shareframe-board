import { HttpResponse } from 'msw';

// Every board endpoint answers with { success, message, data }.
export const ok = (data: unknown = {}, message = '') =>
  HttpResponse.json({ success: true, message, data });

export const fail = (message = 'Fehler', status = 500) =>
  HttpResponse.json({ success: false, message, data: null }, { status });
