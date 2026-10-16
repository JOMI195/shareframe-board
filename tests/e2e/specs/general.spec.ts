import { DASHBOARD_PASSWORD, expectShimCall, isCleared, showsStatus, waitForPanel } from '../support/board';
import { card, expect, signIn, test } from '../support/fixtures';

test('device info shows the identity the server knows and live system metrics', async ({ page, server }) => {
  await expect.poll(async () => (await server.state()).heartbeats.length, { timeout: 20_000 }).toBeGreaterThan(0);
  const serial = (await server.state()).heartbeats.at(-1)!.serial_number as string;

  await signIn(page);
  await page.goto('/general/');
  await expect(card(page, 'Gerät')).toContainText(serial);
  await expect(card(page, 'System')).toContainText('42.5 °C');
});

test('a changed password replaces the initial one', async ({ page, board, request }) => {
  const login = async (password: string) => (await request.post('/api/auth/login', { data: { password } })).status();
  await signIn(page);
  await page.goto('/general/');
  await page.getByRole('button', { name: 'Passwort ändern' }).click();
  const dialog = page.getByRole('dialog');
  await dialog.getByLabel(/Aktuelles Passwort/).fill(DASHBOARD_PASSWORD);
  await dialog.getByLabel(/^Neues Passwort(?! bestätigen)/).fill('changed-e2e');
  await dialog.getByLabel(/Neues Passwort bestätigen/).fill('changed-e2e');
  await dialog.getByRole('button', { name: 'Ändern' }).click();

  try {
    await expect(page.getByText('Passwort erfolgreich geändert')).toBeVisible();
    expect(await login('changed-e2e')).toBe(200);
    expect(await login(DASHBOARD_PASSWORD)).toBe(401);
  } finally {
    await board.postRaw('/api/auth/change-password', { current_password: 'changed-e2e', new_password: DASHBOARD_PASSWORD });
  }
});

test('restarting from the power menu reboots the board', async ({ page }) => {
  await signIn(page);
  await page.getByRole('button', { name: 'Energieoptionen' }).click();
  await page.getByRole('menuitem', { name: 'Neustarten' }).click();
  await page.getByRole('dialog').getByRole('button', { name: 'Neustarten' }).click();
  await expectShimCall('reboot');
  await waitForPanel(showsStatus('reboot'), 'reboot status screen');
});

test('shutting down blanks the panel before powering off', async ({ page }) => {
  await signIn(page);
  await page.getByRole('button', { name: 'Energieoptionen' }).click();
  await page.getByRole('menuitem', { name: 'Herunterfahren' }).click();
  await page.getByRole('dialog').getByRole('button', { name: 'Herunterfahren' }).click();
  await waitForPanel(isCleared, 'panel cleared before shutdown');
  await expectShimCall('shutdown');
});

test('factory reset from the general page schedules the wipe', async ({ page }) => {
  await signIn(page);
  await page.goto('/general/');
  await page.getByRole('button', { name: 'Zurücksetzen' }).click();
  await page.getByRole('dialog').getByRole('button', { name: 'Zurücksetzen' }).click();
  await expectShimCall('shareframe-factory-reset --yes');
});
