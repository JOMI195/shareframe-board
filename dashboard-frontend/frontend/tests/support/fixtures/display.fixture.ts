import type { DisplayStats } from '@/store/displayStats/displayStats.Slice';

export const makeDisplayStats = (over: Partial<DisplayStats> = {}): DisplayStats => ({
  epd_refresh_total: 1200,
  epd_image_refresh_total: 1000,
  epd_clear_total: 200,
  epd_poweron_total: 1200,
  epd_refresh_fail_total: 0,
  epd_poweron_fail_total: 0,
  epd_busy_ms_total: 1_200_000,
  epd_last_refresh_ms: 1000,
  epd_last_refresh_at: 1_790_000_000,
  epd_first_use_at: 1_760_000_000,
  app_boot_total: 12,
  consecutive_failures: 0,
  rated_refreshes: 1_000_000,
  wear_percent: 0.12,
  health: 'ok',
  ...over,
});
