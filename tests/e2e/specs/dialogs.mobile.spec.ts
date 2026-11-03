import { expectShimCall } from '../support/board';
import { expect, signIn, test } from '../support/fixtures';
import { boxOf } from '../support/layout';

test('task dialogs fill the screen with the close X top right', async ({ page }) => {
  await signIn(page);
  await page.goto('/general/');
  await page.getByRole('button', { name: 'Passwort ändern' }).click();

  const dialog = page.getByRole('dialog', { name: 'Geräte-Passwort ändern' });
  const viewport = page.viewportSize()!;
  const box = await boxOf(dialog);
  expect(box.width).toBe(viewport.width);
  expect(box.height).toBe(viewport.height);

  const close = await boxOf(dialog.getByRole('button', { name: 'Schließen' }));
  expect(close.x).toBeGreaterThan(viewport.width / 2);
  expect(close.y).toBeLessThan(80);
  expect(close.width).toBeGreaterThanOrEqual(40);

  await dialog.getByRole('button', { name: 'Schließen' }).click();
  await expect(dialog).toBeHidden();
});

test('the back button closes a dialog without leaving the page', async ({ page }) => {
  await signIn(page);
  await page.goto('/general/');
  await page.getByRole('button', { name: 'Passwort ändern' }).click();
  const dialog = page.getByRole('dialog', { name: 'Geräte-Passwort ändern' });
  await expect(dialog).toBeVisible();

  await page.goBack();

  await expect(dialog).toBeHidden();
  await expect(page).toHaveURL(/\/general\/$/);
});

test('questions open as a compact dialog from the side menu', async ({ page }) => {
  await signIn(page);
  await page.getByRole('button', { name: 'Menü öffnen' }).click();
  await page.getByRole('navigation', { name: 'Hauptmenü' }).getByRole('button', { name: 'Energie' }).click();
  await page.getByRole('menuitem', { name: 'Neu starten' }).click();

  const confirm = page.getByRole('dialog', { name: 'Bilderrahmen neu starten?' });
  expect((await boxOf(confirm)).width).toBeLessThan(page.viewportSize()!.width);
  await expect(confirm.getByRole('button', { name: 'Neu starten' })).toBeFocused();

  await confirm.getByRole('button', { name: 'Abbrechen' }).click();
  await expect(confirm).toBeHidden();
});

test('Enter on the keyboard submits a form dialog', async ({ page }) => {
  await signIn(page);
  await page.goto('/network/');
  await page.getByRole('button', { name: 'WLAN hinzufügen' }).click();
  const dialog = page.getByRole('dialog', { name: 'WLAN hinzufügen' });

  await dialog.getByLabel(/Netzwerkname/).fill('Garden');
  await dialog.getByLabel(/^Passwort$/).fill('gardenpass');
  await dialog.getByLabel(/^Passwort$/).press('Enter');

  await expectShimCall('shareframe-wifi add Garden gardenpass');
  await expect(dialog).toBeHidden();
});
