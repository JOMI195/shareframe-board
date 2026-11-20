import { DASHBOARD_PASSWORD, expectSavedNetwork, setWifiMode } from '../support/board';
import { card, expect, test } from '../support/fixtures';
import { expectNoHorizontalScroll } from '../support/layout';

// Setting up Wi-Fi from a phone joined to the frame's hotspot, the most common first contact.

test.beforeEach(async ({ page }) => {
  await setWifiMode('ap');
  await page.goto('/');
  await expect(page).toHaveURL(/\/auth\/sign-in$/);
});

test('signs in with the device password and adds a WLAN on its own page', async ({ page }) => {
  await expectNoHorizontalScroll(page);
  await page.getByRole('button', { name: 'Mit Geräte-Passwort anmelden' }).click();
  await page.getByRole('textbox', { name: 'Passwort', exact: true }).fill(DASHBOARD_PASSWORD);
  await page.getByRole('button', { name: 'Anmelden', exact: true }).click();
  await expect(page).toHaveURL(/\/network\/$/);
  await expect(page.getByRole('heading', { level: 2, name: 'WLAN einrichten' })).toBeVisible();
  await expectNoHorizontalScroll(page);

  await page.getByRole('button', { name: 'WLAN hinzufügen' }).click();
  await expect(page).toHaveURL(/\/network\/add\/$/);
  const form = card(page, 'WLAN hinzufügen');
  await expectNoHorizontalScroll(page);

  await form.getByLabel(/Netzwerkname/).fill('HomeNet');
  await form.getByLabel(/^Passwort$/).fill('homepass1');
  await form.getByRole('button', { name: 'Speichern & verbinden' }).click();

  await expectSavedNetwork('HomeNet', 'homepass1');
  await expect(page).toHaveURL(/\/network\/$/);
  await expect(page.getByText(/WLAN gespeichert/)).toBeInViewport();
  await expect(card(page, 'WLAN einrichten').getByText(/Der Rahmen verbindet sich/)).toBeVisible();
});

test('asks before switching to the WLAN and the back button keeps the hotspot', async ({ page }) => {
  await page.getByRole('button', { name: 'Mit Geräte-Passwort anmelden' }).click();
  await page.getByRole('textbox', { name: 'Passwort', exact: true }).fill(DASHBOARD_PASSWORD);
  await page.getByRole('button', { name: 'Anmelden', exact: true }).click();
  await expect(page).toHaveURL(/\/network\/$/);

  await card(page, 'WLAN einrichten').getByRole('button', { name: 'Jetzt verbinden' }).click();
  const confirm = page.getByRole('dialog', { name: 'Mit WLAN verbinden?' });
  await expect(confirm.getByText('http://shareframe-board-0e2e.local')).toBeVisible();
  await expect(confirm.getByRole('button', { name: 'Verbinden' })).toBeFocused();

  await page.goBack();

  await expect(confirm).toBeHidden();
  await expect(page).toHaveURL(/\/network\/$/);
  await expect(card(page, 'WLAN einrichten').getByText(/Der Rahmen verbindet sich/)).toHaveCount(0);
});

test('a forgotten password opens a page that offers the setup code first and leads back to the sign-in', async ({ page }) => {
  await page.getByRole('link', { name: 'Passwort vergessen?' }).click();

  await expect(page.getByRole('heading', { level: 1, name: 'Passwort vergessen?' })).toBeVisible();
  await expect(page.getByText(/mit dem Einrichtungscode .* So bleibt alles erhalten/)).toBeInViewport();
  await expect(page.getByRole('dialog')).toHaveCount(0);
  await expectNoHorizontalScroll(page);

  await page.getByRole('button', { name: 'Zur Anmeldung' }).click();
  await expect(page).toHaveURL(/\/auth\/sign-in$/);
});
