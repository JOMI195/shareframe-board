import { http } from 'msw';
import * as api from '@/assets/endpoints/api/frame';
import { makeDisplayStats, makeSlideshowStatus } from '../../fixtures';
import { apiUrl } from '../apiUrl';
import { ok } from '../envelope';

// The toggle thunk polls the status until it flips, so the default world must remember it.
const initialBoard = () => ({ slideshowActive: true, displayQuality: 'mono' as 'mono' | 'gray4' });

const board = initialBoard();

export const resetBoard = () => Object.assign(board, initialBoard());

export const setBoard = (over: Partial<typeof board>) => Object.assign(board, over);

export const frameHandlers = [
  http.get(apiUrl(api.getSlideshowStatusUrl()), () => ok(makeSlideshowStatus({ active: board.slideshowActive }))),
  http.post(apiUrl(api.getSlideshowUrl()), async ({ request }) => {
    const { action } = (await request.json()) as { action: 'start' | 'stop' };
    board.slideshowActive = action === 'start';
    return ok({ action });
  }),
  http.post(apiUrl(api.getSlideshowIntervalUrl()), async ({ request }) => {
    const { interval_seconds } = (await request.json()) as { interval_seconds: number };
    return ok({ interval_seconds });
  }),
  http.post(apiUrl(api.getSlideshowNightModeUrl()), () => ok()),
  http.post(apiUrl(api.getSkipSlideshowImageUrl()), () => ok()),
  http.post(apiUrl(api.getClearDisplayUrl()), () => ok()),
  http.get(apiUrl(api.getDisplayStatsUrl()), () => ok(makeDisplayStats())),
  http.get(apiUrl(api.getDisplayQualityUrl()), () => ok({ quality: board.displayQuality })),
  http.post(apiUrl(api.getDisplayQualityUrl()), async ({ request }) => {
    const { quality } = (await request.json()) as { quality: 'mono' | 'gray4' };
    board.displayQuality = quality;
    return ok({ quality });
  }),
];
