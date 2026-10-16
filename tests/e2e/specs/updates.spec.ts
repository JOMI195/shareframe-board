import { expectShimCall, INSTALLED_VERSION, showsStatus, waitForPanel } from '../support/board';
import { expect, signIn, test } from '../support/fixtures';

// Runs last (alphabetical): a successful install leaves the update service waiting
// for the reboot, so that test restarts it at the end.

test('the installed version is shown and no update is offered without a release', async ({ page }) => {
  await signIn(page);
  await page.goto('/updates/');
  await expect(page.getByText(INSTALLED_VERSION).first()).toBeVisible();
  await expect(page.getByText('Keine Updates verfügbar')).toBeVisible();
});

test('an older release is not offered', async ({ page, server }) => {
  await server.offerRelease({ version: '1.1.0' });
  await signIn(page);
  await page.goto('/updates/');
  await expect(page.getByText('Keine Updates verfügbar')).toBeVisible();
});

test('a newer release is announced with its notes', async ({ page, server }) => {
  await server.offerRelease({ version: '9.9.0', release_notes: 'Schärfere Bilder' });
  await signIn(page);
  await page.getByRole('button', { name: 'Benachrichtigungen' }).click();
  await expect(page.getByRole('menu')).toContainText('Neue Version (9.9.0) verfügbar');

  await page.goto('/updates/');
  await expect(page.getByText('Neue Version verfügbar')).toBeVisible();
  await expect(page.getByText('Schärfere Bilder')).toBeVisible();
});

test('a bundle that fails verification is not installed and lands in the history', async ({ page, server, board }) => {
  await server.offerRelease({ version: '9.9.1', corrupt: true });
  await board.post('/api/system/updates/perform-update');
  await expect.poll(async () => (await board.get('/api/system/updates/status')).phase).toBe('failed');
  expect((await server.state()).bundleDownloads).toContainEqual({ name: 'shareframe-9.9.1.raucb', authorized: true });

  await signIn(page);
  await page.goto('/updates/');
  await expect(page.getByText('Update fehlgeschlagen')).toBeVisible();
  await expect(page.getByText(/Checksum mismatch/)).toBeVisible();

  await page.goto('/update-history/');
  await expect(page.getByText(`${INSTALLED_VERSION} → 9.9.1`)).toBeVisible();
  await expect(page.getByText('Installation fehlgeschlagen')).toBeVisible();
});

test('installing from the dashboard downloads, verifies and hands over to rauc', async ({ page, server, board }) => {
  await server.offerRelease({ version: '9.9.2' });
  await signIn(page);
  await page.goto('/updates/');
  await page.getByRole('button', { name: 'Jetzt installieren' }).click();
  await page.getByRole('dialog').getByRole('button', { name: 'Installieren' }).click();

  await expectShimCall('rauc install /data/cache/shareframe-9.9.2.raucb');
  await expectShimCall('s6-linux-init-shutdown -r now');
  await waitForPanel(showsStatus('update'), 'update status screen');
  expect((await board.get('/api/system/updates/status')).phase).toBe('awaiting-reboot');
  await board.restartService('update');
});
