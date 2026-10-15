import { expectShimCall, shimCalls } from '../support/board';
import { card, expect, signIn, test } from '../support/fixtures';

const savedNetworks = (page: import('@playwright/test').Page) =>
  card(page, 'Übersicht (WIFI)').getByRole('listitem');

test.beforeEach(async ({ page }) => {
  await signIn(page);
  await page.goto('/network/');
});

test('shows the current and saved networks but never protected ones', async ({ page }) => {
  await expect(page.getByText('E2E-WiFi').first()).toBeVisible();
  await expect(savedNetworks(page)).toHaveText(['E2E-WiFi', 'Neighbour']);
});

test('adding a network saves it on the board', async ({ page }) => {
  await page.getByRole('button', { name: 'Netzwerk hinzufügen' }).click();
  const dialog = page.getByRole('dialog');
  await dialog.getByLabel(/Netzwerkname/).fill('Garden');
  await dialog.getByLabel(/^Passwort/).fill('gardenpass');
  await dialog.getByRole('button', { name: 'Hinzufügen' }).click();

  await expect(page.getByText('Netzwerk erfolgreich hinzugefügt')).toBeVisible();
  await expectShimCall('shareframe-wifi add Garden gardenpass');
  await expect(savedNetworks(page)).toContainText(['Garden']);
});

test('forgetting a network removes it from the board', async ({ page }) => {
  await savedNetworks(page).filter({ hasText: 'Neighbour' }).getByRole('button', { name: 'delete' }).click();
  await page.getByRole('dialog').getByRole('button', { name: 'Entfernen' }).click();

  await expect(page.getByText('Netzwerk erfolgreich entfernt')).toBeVisible();
  await expectShimCall('shareframe-wifi del Neighbour');
  await expect(savedNetworks(page)).toHaveText(['E2E-WiFi']);
});

test('the network the board is connected to cannot be forgotten', async ({ page }) => {
  await savedNetworks(page).filter({ hasText: 'E2E-WiFi' }).getByRole('button', { name: 'delete' }).click();
  await page.getByRole('dialog').getByRole('button', { name: 'Entfernen' }).click();

  await expect(page.getByText('Entfernen des Netzwerks fehlgeschlagen')).toBeVisible();
  expect(await shimCalls()).not.toContain('shareframe-wifi del E2E-WiFi');
});

test('the hotspot password can be revealed and changed', async ({ page }) => {
  const ap = card(page, 'Access Point');
  await ap.getByRole('button', { name: 'AP-Passwort anzeigen' }).click();
  await expect(ap.getByText('hotspot123')).toBeVisible();

  await page.getByRole('button', { name: 'AP-Passwort ändern' }).click();
  const dialog = page.getByRole('dialog');
  await dialog.getByLabel(/Neues AP-Passwort/).fill('newhotspot');
  await dialog.getByLabel(/Bestätigen/).fill('newhotspot');
  await dialog.getByRole('button', { name: 'Ändern' }).click();

  await expect(page.getByText('AP-Passwort erfolgreich geändert')).toBeVisible();
  await expectShimCall('shareframe-ap setpass newhotspot');
});
