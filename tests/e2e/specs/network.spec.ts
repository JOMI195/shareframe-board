import { expectShimCall, shimCalls } from '../support/board';
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
  await expectShimCall('shareframe-wifi add Garden gardenpass');
  await expect(savedNetworks(page)).toContainText(['Garden']);
});

test('forgetting a network removes it from the board', async ({ page }) => {
  await savedNetworks(page).getByRole('button', { name: '„Neighbour“ entfernen' }).click();
  await page.getByRole('dialog').getByRole('button', { name: 'Entfernen' }).click();

  await expect(page.getByText('WLAN entfernt')).toBeVisible();
  await expectShimCall('shareframe-wifi del Neighbour');
  await expect(savedNetworks(page)).toHaveText(['E2E-WiFi']);
});

test('the network the board is connected to cannot be forgotten', async ({ page }) => {
  await savedNetworks(page).getByRole('button', { name: '„E2E-WiFi“ entfernen' }).click();
  await page.getByRole('dialog').getByRole('button', { name: 'Entfernen' }).click();

  await expect(page.getByText('WLAN konnte nicht entfernt werden')).toBeVisible();
  expect(await shimCalls()).not.toContain('shareframe-wifi del E2E-WiFi');
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
  await expectShimCall('shareframe-ap setpass newhotspot');
});
