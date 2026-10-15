import { expectShimCall, readPanel, showsDefault, waitForPanel } from '../support/board';
import { card, expect, signIn, test } from '../support/fixtures';

test('every service reports as running', async ({ page }) => {
  await signIn(page);
  await page.goto('/services/');
  await expect(page.getByText('Aktiv')).toHaveCount(5);
});

test('restarting a service from the overview restarts the process', async ({ page, board }) => {
  const before = (await board.services()).find((s) => s.id === 'heartbeat')!.pid;
  await signIn(page);
  await page.goto('/services/');

  await card(page, 'Heartbeat').getByRole('button', { name: 'Neustart' }).click();
  await page.getByRole('dialog').getByRole('button', { name: 'Neustart' }).click();
  await expect(page.getByText('Dienst wird neu gestartet')).toBeVisible();

  await expectShimCall('s6-svc -r /run/service/shareframe-heartbeat');
  await expect
    .poll(async () => {
      const svc = (await board.services()).find((s) => s.id === 'heartbeat');
      return svc?.running && svc.pid !== before;
    })
    .toBe(true);
});

test('a service page shows its process and its own log', async ({ page, board }) => {
  const display = (await board.services()).find((s) => s.id === 'display')!;
  await signIn(page);
  await page.goto('/services/display');

  await expect(card(page, 'Display')).toContainText(String(display.pid));
  await expect(page.getByText(/\[mock\] Display/).first()).toBeVisible();
});

test('the display page switches photos to 4 gray levels', async ({ page, board }) => {
  await signIn(page);
  await page.goto('/services/display');
  const toggle = card(page, 'Bildqualität').getByRole('switch', { name: '4 Graustufen (Test)' });

  try {
    await toggle.click();
    await expect(page.getByText('4 Graustufen ab dem nächsten Bild')).toBeVisible();
    await expect.poll(async () => (await board.get('/api/frame/display/quality')).quality).toBe('gray4');

    const before = (await readPanel())!;
    expect(before.gray4).toBe(false);
    await board.post('/api/frame/slideshow/skip');
    await waitForPanel((p) => showsDefault(p) && p.seq > before.seq && p.gray4, 'next photo painted in 4-gray');
  } finally {
    await board.post('/api/frame/display/quality', { quality: 'mono' });
  }
});
