import { codeOn, DASHBOARD_PASSWORD, expectShimCall, factoryResetScheduled, isCleared, showsStatus, waitForPanel } from '../support/board';
import { card, expect, signIn, test } from '../support/fixtures';

test('device info shows the identity the server knows and live system metrics', async ({ page, server }) => {
  await expect.poll(async () => (await server.state()).heartbeats.length, { timeout: 20_000 }).toBeGreaterThan(0);
  const serial = (await server.state()).heartbeats.at(-1)!.serial_number as string;

  await signIn(page);
  await page.goto('/general/');
  await expect(card(page, 'Gerät')).toContainText(serial);
  await expect(card(page, 'System')).toBeHidden();
  await page.getByRole('switch', { name: 'Expertenmodus' }).check();
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
  await dialog.getByRole('button', { name: 'Passwort ändern' }).click();

  try {
    await expect(page.getByText('Passwort geändert')).toBeVisible();
    expect(await login('changed-e2e')).toBe(200);
    expect(await login(DASHBOARD_PASSWORD)).toBe(401);
  } finally {
    // A changed password ends every other session, the fixture's too: sign in again to put it back.
    await board.login('changed-e2e');
    await board.postRaw('/api/auth/change-password', { current_password: 'changed-e2e', new_password: DASHBOARD_PASSWORD });
  }
});

test('restarting from the power menu reboots the board', async ({ page }) => {
  await signIn(page);
  await page.getByRole('button', { name: 'Energieoptionen' }).click();
  await page.getByRole('menuitem', { name: 'Neu starten' }).click();
  await page.getByRole('dialog').getByRole('button', { name: 'Neu starten' }).click();
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

test('factory reset from the general page needs the code from the panel, also when signed in', async ({ page }) => {
  await signIn(page);
  await page.goto('/general/');
  await page.getByRole('button', { name: 'Zurücksetzen' }).click();
  await page.getByRole('dialog').getByRole('button', { name: 'Zurücksetzen' }).click();
  const code = codeOn(await waitForPanel(showsStatus('reset_code'), 'reset code on the panel'));
  const entry = page.getByRole('dialog', { name: 'Code vom Bilderrahmen eingeben' });
  await entry.getByLabel('Code').fill(code);
  await entry.getByRole('button', { name: 'Endgültig zurücksetzen' }).click();
  await expect.poll(factoryResetScheduled, { message: 'wipe marked for the next boot' }).toBe(true);
  await expectShimCall('reboot');
});

test('diagnostics reach the server with the board state and without credentials', async ({ page, server }) => {
  await signIn(page);
  await page.goto('/general/');
  await page.getByRole('button', { name: 'Diagnose senden' }).click();
  await expect(page.getByText('Diagnose an den Support gesendet')).toBeVisible();

  const [sent] = await server.diagnostics();
  expect(sent.trigger).toBe('owner');
  for (const section of ['release', 'sysinfo', 'services', 'events', 'syslog'])
    expect(sent.text, `section ${section}`).toContain(`===== ${section} =====`);
  expect(sent.text).toContain('shareframe-display: up (pid');
  expect(sent.text).toContain('e2e kernel: wlan0 up');
  expect(sent.text).not.toContain('CANARY-PSK-1234');
});
