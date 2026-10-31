import { DASHBOARD_PASSWORD, expectShimCall, setWifiMode } from '../support/board';
import { card, expect, test } from '../support/fixtures';
import { boxOf, expectNoHorizontalScroll } from '../support/layout';

// Setting up Wi-Fi from a phone joined to the frame's hotspot, the most common first contact.

test.beforeEach(async ({ page }) => {
  await setWifiMode('ap');
  await page.goto('/');
  await expect(page).toHaveURL(/\/auth\/sign-in$/);
});

test('signs in with the device password and adds a WLAN in a fullscreen dialog', async ({ page }) => {
  await expectNoHorizontalScroll(page);
  await page.getByRole('textbox', { name: 'Passwort', exact: true }).fill(DASHBOARD_PASSWORD);
  await page.getByRole('button', { name: 'Anmelden', exact: true }).click();
  await expect(page).toHaveURL(/\/network\/$/);
  await expect(page.getByText('Hotspot aktiv')).toBeVisible();
  await expectNoHorizontalScroll(page);

  await page.getByRole('button', { name: 'WLAN hinzufügen' }).click();
  const dialog = page.getByRole('dialog', { name: 'WLAN hinzufügen' });
  const viewport = page.viewportSize()!;
  expect((await boxOf(dialog)).width).toBe(viewport.width);

  await dialog.getByLabel(/Netzwerkname/).fill('HomeNet');
  await dialog.getByLabel(/^Passwort$/).fill('homepass1');
  await dialog.getByRole('button', { name: 'Hinzufügen' }).click();

  await expectShimCall('shareframe-wifi add HomeNet homepass1');
  await expect(dialog).toBeHidden();
  await expect(page.getByText(/WLAN gespeichert/)).toBeInViewport();
});

test('asks before switching to the WLAN and the back button keeps the hotspot', async ({ page }) => {
  await page.getByRole('textbox', { name: 'Passwort', exact: true }).fill(DASHBOARD_PASSWORD);
  await page.getByRole('button', { name: 'Anmelden', exact: true }).click();
  await expect(page).toHaveURL(/\/network\/$/);

  await card(page, 'WLAN einrichten').getByRole('button', { name: 'Jetzt verbinden' }).click();
  const confirm = page.getByRole('dialog', { name: 'Mit WLAN verbinden?' });
  await expect(confirm.getByText('http://shareframe-e2e.local')).toBeVisible();
  await expect(confirm.getByRole('button', { name: 'Verbinden' })).toBeFocused();

  await page.goBack();

  await expect(confirm).toBeHidden();
  await expect(page).toHaveURL(/\/network\/$/);
  await expect(card(page, 'WLAN einrichten').getByText(/Der Rahmen verbindet sich/)).toHaveCount(0);
});

test('a forgotten password leads to a factory reset that starts on Abbrechen', async ({ page }) => {
  await page.getByRole('button', { name: /Auf Werkseinstellungen zurücksetzen/ }).click();
  const confirm = page.getByRole('dialog', { name: 'Auf Werkseinstellungen zurücksetzen?' });

  await expect(confirm.getByRole('button', { name: 'Abbrechen' })).toBeFocused();
  expect((await boxOf(confirm)).width).toBeLessThan(page.viewportSize()!.width);

  await confirm.getByRole('button', { name: 'Abbrechen' }).click();
  await expect(confirm).toBeHidden();
  await expect(page).toHaveURL(/\/auth\/sign-in$/);
});
