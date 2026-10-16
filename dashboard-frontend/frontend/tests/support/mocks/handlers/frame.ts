import { http, HttpResponse } from 'msw';
import * as api from '@/assets/endpoints/api/frame';
import { makeDisplayStats, makeSlideshowImages, makeSlideshowStatus, type SlideshowImagesPayload } from '../../fixtures';
import { apiPath, apiUrl } from '../apiUrl';
import { ok } from '../envelope';

// The toggle thunk polls the status until it flips, so the default world must remember it.
const initialBoard = () => ({
  slideshowActive: true,
  slideshowImages: makeSlideshowImages() as SlideshowImagesPayload,
});

const board = initialBoard();

export const resetBoard = () => Object.assign(board, initialBoard());

export const setBoard = (over: Partial<typeof board>) => Object.assign(board, over);

const image = (type: string) => new HttpResponse(new Uint8Array([0x42, 0x4d]), { headers: { 'Content-Type': type } });

// Mirrors the board: a custom order puts its ids first and keeps the rest behind them.
const applyOrder = (current: SlideshowImagesPayload, mode: SlideshowImagesPayload['mode'], customOrder?: number[]): SlideshowImagesPayload => {
  const order = customOrder ?? current.custom_order;
  const byArrival = [...current.images].sort((a, b) => a.received_at - b.received_at);
  const sequence = mode === 'newest_first' ? byArrival.reverse()
    : mode === 'custom' ? [
      ...order.flatMap((id) => byArrival.filter((img) => img.id === id)),
      ...byArrival.filter((img) => !order.includes(img.id)),
    ]
      : byArrival;
  return { ...current, mode, custom_order: order, images: sequence.map((img, i) => ({ ...img, position: i + 1 })) };
};

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
  http.post(apiUrl(api.getShowSlideshowImageUrl()), () => ok()),
  http.post(apiUrl(api.getClearDisplayUrl()), () => ok()),
  http.get(apiUrl(api.getDisplayStatsUrl()), () => ok(makeDisplayStats())),
  http.get(apiUrl(api.getSlideshowImagesUrl()), () => ok(board.slideshowImages)),
  http.get(apiPath(api.getImageFileUrl(0)), () => image('image/jpeg')),
  http.post(apiUrl(api.getSlideshowOrderUrl()), async ({ request }) => {
    const { mode, custom_order } = (await request.json()) as { mode: SlideshowImagesPayload['mode']; custom_order?: number[] };
    board.slideshowImages = applyOrder(board.slideshowImages, mode, custom_order);
    return ok({ mode, custom_order: board.slideshowImages.custom_order });
  }),
];
