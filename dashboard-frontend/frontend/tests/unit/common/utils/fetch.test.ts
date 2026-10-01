import { afterEach, describe, expect, it, vi } from 'vitest';
import { fetchWithTimeout } from '@/common/utils/fetch';

const stubFetch = (impl: typeof fetch) => {
  const spy = vi.fn(impl);
  vi.stubGlobal('fetch', spy);
  return spy;
};

describe('fetchWithTimeout', () => {
  afterEach(() => {
    vi.unstubAllGlobals();
    vi.useRealTimers();
  });

  it('prefixes the dev API base and sends cookies', async () => {
    const spy = stubFetch(async () => new Response('{}'));

    await fetchWithTimeout('/api/system/health', { method: 'POST' });

    const [url, init] = spy.mock.calls[0];
    expect(url).toBe('http://localhost:8080/api/system/health');
    expect(init).toMatchObject({ method: 'POST', credentials: 'include' });
    expect(init?.signal).toBeInstanceOf(AbortSignal);
  });

  it('aborts the request once the timeout elapses', async () => {
    vi.useFakeTimers();
    stubFetch((_url, init) =>
      new Promise((_resolve, reject) => {
        init?.signal?.addEventListener('abort', () => reject(new DOMException('Aborted', 'AbortError')));
      }),
    );

    const pending = fetchWithTimeout('/api/system/health', {}, 1000);
    vi.advanceTimersByTime(1000);

    await expect(pending).rejects.toThrow('Aborted');
  });

  it('rethrows network errors', async () => {
    stubFetch(async () => {
      throw new TypeError('Failed to fetch');
    });

    await expect(fetchWithTimeout('/api/system/health')).rejects.toThrow('Failed to fetch');
  });
});
