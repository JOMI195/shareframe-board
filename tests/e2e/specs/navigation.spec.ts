import { expect, signIn, test, trackApiErrors } from '../support/fixtures';

// Every page against the real board: it renders and none of its API calls fail,
// which catches a renamed route or field on either side.
const pages: [string, string | RegExp][] = [
  ['/', 'Bilderwiedergabe'],
  ['/slideshow/', 'Wiedergabereihenfolge'],
  ['/customization/', 'Name des Absenders anzeigen'],
  ['/network/', 'Aktuelles Netzwerk'],
  ['/updates/', 'Keine Updates verfügbar'],
  ['/update-history/', 'Update-Verlauf'],
  ['/general/', 'Hardwarespezifikation'],
  ['/display-health/', 'Zuverlässigkeit'],
  ['/logs/', 'e2e kernel: boot ok'],
  ['/services/', 'Heartbeat'],
  ['/services/display', 'PID'],
  ['/services/websocket', 'PID'],
  ['/services/dashboard', 'PID'],
  ['/services/heartbeat', 'PID'],
  ['/services/update', 'PID'],
];

test('every page loads without a failing API call', async ({ page }) => {
  test.setTimeout(pages.length * 10_000);
  await signIn(page);
  for (const [path, marker] of pages) {
    await test.step(path, async () => {
      const failures = trackApiErrors(page);
      await page.goto(path);
      await expect(page.getByText(marker).first()).toBeVisible();
      await page.waitForTimeout(1500);
      expect(failures, path).toEqual([]);
    });
  }
});

test('unknown pages show a 404', async ({ page }) => {
  await signIn(page);
  await page.goto('/does-not-exist');
  await expect(page.getByText('Page not found')).toBeVisible();
});
