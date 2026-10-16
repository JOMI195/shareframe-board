import type { NightStyle } from '@/store/slideshowOperation/slideshowOperation.Slice';

// Wire shape of GET /api/frame/slideshow/status `data`, not the slice state.
export interface SlideshowStatusPayload {
  active: boolean;
  loop_started: boolean;
  refreshing: boolean;
  image_count: number;
  interval_seconds: number;
  seconds_until_next: number;
  night_mode: {
    enabled: boolean;
    start_hour: number;
    end_hour: number;
    interval_seconds: number;
    style: NightStyle;
    active_now: boolean;
  };
  now_showing: {
    seq: number;
    state: 'empty' | 'image' | 'cleared';
    image_id: number;
    sender: string;
  };
}

export const makeSlideshowStatus = (over: Partial<SlideshowStatusPayload> = {}): SlideshowStatusPayload => ({
  active: true,
  loop_started: true,
  refreshing: false,
  image_count: 12,
  interval_seconds: 900,
  seconds_until_next: 300,
  night_mode: { enabled: false, start_hour: 2, end_hour: 5, interval_seconds: 3600, style: 'slower', active_now: false },
  now_showing: { seq: 1, state: 'image', image_id: 0, sender: '' },
  ...over,
});
