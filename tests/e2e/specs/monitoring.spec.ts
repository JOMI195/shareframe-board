import { card, expect, signIn, test } from '../support/fixtures';

test('display health counts the refreshes the panel really did', async ({ page, board }) => {
  const before = (await board.get('/api/frame/display/stats')).epd_image_refresh_total;
  await board.skip();
  await expect.poll(async () => (await board.get('/api/frame/display/stats')).epd_image_refresh_total).toBe(before + 1);

  await signIn(page);
  await page.goto('/display-health/');
  await expect(card(page, 'Zustand')).toContainText('OK');
  await expect(card(page, 'Verschleiß')).toContainText(String(before + 1));
});

test('the logs page shows the board system log', async ({ page }) => {
  await signIn(page);
  await page.goto('/logs/');
  await expect(page.getByText('e2e kernel: wlan0 up')).toBeVisible();
});
