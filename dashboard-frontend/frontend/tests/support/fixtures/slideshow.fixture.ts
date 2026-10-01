// Wire shape of GET /api/frame/slideshow/status `data`, not the slice state.
export interface SlideshowStatusPayload {
  active: boolean;
  loop_started: boolean;
  image_count: number;
  interval_seconds: number;
  seconds_until_next: number;
  night_mode: {
    enabled: boolean;
    start_hour: number;
    end_hour: number;
    interval_seconds: number;
    active_now: boolean;
  };
}

export const makeSlideshowStatus = (over: Partial<SlideshowStatusPayload> = {}): SlideshowStatusPayload => ({
  active: true,
  loop_started: true,
  image_count: 12,
  interval_seconds: 900,
  seconds_until_next: 300,
  night_mode: { enabled: false, start_hour: 2, end_hour: 5, interval_seconds: 3600, active_now: false },
  ...over,
});
