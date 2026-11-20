import { apConf, expectSavedNetwork, savedWifiNetworks, wifiConf } from '../support/board';
import { card, expect, signIn, test } from '../support/fixtures';

const savedNetworks = (page: import('@playwright/test').Page) =>
  card(page, 'WLAN').getByRole('listitem');

test.beforeEach(async ({ page }) => {
  await signIn(page);
  await page.goto('/network/');
});

test('shows the current and saved networks but never protected ones', async ({ page }) => {
  await expect(page.getByText('E2E-WiFi').first()).toBeVisible();
  await expect(savedNetworks(page)).toHaveText(['E2E-WiFi', 'Neighbour']);
});

test('adding a network saves it on the board', async ({ page }) => {
  await page.getByRole('button', { name: 'WLAN hinzufügen' }).click();
  const dialog = page.getByRole('dialog');
  await dialog.getByLabel(/Netzwerkname/).fill('Garden');
  await dialog.getByLabel(/^Passwort$/).fill('gardenpass');
  await dialog.getByRole('button', { name: 'Hinzufügen' }).click();

  await expect(page.getByText('WLAN hinzugefügt')).toBeVisible();
  await expectSavedNetwork('Garden', 'gardenpass');
  await expect(savedNetworks(page)).toContainText(['Garden']);
});

test('an open network is saved without a password, and saving it again replaces it', async ({ page }) => {
  const add = async (secret?: string) => {
    await page.getByRole('button', { name: 'WLAN hinzufügen' }).click();
    const dialog = page.getByRole('dialog', { name: 'WLAN hinzufügen' });
    await dialog.getByLabel(/Netzwerkname/).fill('Cafe Open');
    if (secret) await dialog.getByLabel(/^Passwort$/).fill(secret);
    else await dialog.getByLabel('Offenes WLAN ohne Passwort').check();
    await dialog.getByRole('button', { name: 'Hinzufügen' }).click();
    await expect(dialog).toBeHidden();
  };
  const copies = async () => (await savedWifiNetworks()).filter((s) => s === 'Cafe Open').length;

  await add();
  await expect.poll(copies).toBe(1);
  expect(await wifiConf()).toMatch(/key_mgmt=NONE/);

  await add('cafepass1');
  await expect.poll(async () => /key_mgmt=NONE/.test(await wifiConf())).toBe(false);
  expect(await copies()).toBe(1);
  await expectSavedNetwork('Cafe Open', 'cafepass1');
});

test('forgetting a network removes it from the board', async ({ page }) => {
  await savedNetworks(page).getByRole('button', { name: '„Neighbour“ entfernen' }).click();
  await page.getByRole('dialog').getByRole('button', { name: 'Entfernen' }).click();

  await expect(page.getByText('WLAN entfernt')).toBeVisible();
  await expect.poll(savedWifiNetworks, { message: 'Neighbour gone from the board' }).not.toContain('Neighbour');
  await expect(savedNetworks(page)).toHaveText(['E2E-WiFi']);
});

test('the network the board is connected to cannot be forgotten', async ({ page }) => {
  await savedNetworks(page).getByRole('button', { name: '„E2E-WiFi“ entfernen' }).click();
  await page.getByRole('dialog').getByRole('button', { name: 'Entfernen' }).click();

  await expect(page.getByText('Mit diesem WLAN ist der Bilderrahmen gerade verbunden, deshalb kann es nicht entfernt werden.')).toBeVisible();
  expect(await savedWifiNetworks()).toContain('E2E-WiFi');
});

test('the hotspot password can be revealed and changed', async ({ page }) => {
  const ap = card(page, 'Hotspot');
  await ap.getByRole('button', { name: 'Hotspot-Passwort anzeigen' }).click();
  await expect(ap.getByText('hotspot123')).toBeVisible();

  await page.getByRole('button', { name: 'Hotspot-Passwort ändern' }).click();
  const dialog = page.getByRole('dialog');
  await dialog.getByLabel(/^Neues Hotspot-Passwort$/).fill('newhotspot');
  await dialog.getByLabel(/Hotspot-Passwort bestätigen/).fill('newhotspot');
  await dialog.getByRole('button', { name: 'Passwort ändern' }).click();

  await expect(page.getByText('Hotspot-Passwort geändert')).toBeVisible();
  await expect.poll(apConf, { message: 'new password stored for the hotspot' }).toContain("AP_PASS='newhotspot'");
});

test('a network named with accents is shown and forgotten by that name', async ({ page }) => {
  await page.getByRole('button', { name: 'WLAN hinzufügen' }).click();
  const dialog = page.getByRole('dialog');
  await dialog.getByLabel(/Netzwerkname/).fill('Café Wi-Fi');
  await dialog.getByLabel(/^Passwort$/).fill('cafepass1');
  await dialog.getByRole('button', { name: 'Hinzufügen' }).click();
  await expectSavedNetwork('Café Wi-Fi', 'cafepass1');
  await expect(savedNetworks(page)).toContainText(['Café Wi-Fi']);

  await savedNetworks(page).getByRole('button', { name: '„Café Wi-Fi“ entfernen' }).click();
  await page.getByRole('dialog').getByRole('button', { name: 'Entfernen' }).click();
  await expect(page.getByText('WLAN entfernt')).toBeVisible();
  await expect.poll(savedWifiNetworks, { message: 'gone from the board' }).not.toContain('Café Wi-Fi');
});
