import { DASHBOARD_PASSWORD } from '../support/board';
import { expect, test } from '../support/fixtures';
import { boxOf, expectNoHorizontalScroll } from '../support/layout';

test('signs in with a one-time code on a phone', async ({ page }) => {
  await page.goto('/auth/sign-in');
  await expectNoHorizontalScroll(page);
  const code = page.getByRole('textbox', { name: 'Einmal-Code', exact: true });
  await expect(code).toHaveAttribute('autocomplete', 'one-time-code');

  await code.fill('123456');
  await page.getByRole('button', { name: 'Anmelden', exact: true }).click();

  await expect(page).toHaveURL(/\/$/);
  await expect(page).toHaveTitle('Steuerung – ShareFrame');
});

test('a wrong password clears the field for another try', async ({ page }) => {
  await page.goto('/auth/sign-in');
  await page.getByRole('button', { name: 'Mit Geräte-Passwort anmelden' }).click();
  const password = page.getByRole('textbox', { name: 'Passwort', exact: true });
  await password.fill('wrong-password');
  // Slow enough for the loading snackbar to show.
  await page.route('**/api/auth/login', async (route) => {
    await new Promise((resolve) => setTimeout(resolve, 2000));
    await route.continue();
  });
  await page.getByRole('button', { name: 'Anmelden', exact: true }).click();

  const loading = page.getByRole('status').filter({ hasText: 'Du wirst angemeldet' });
  await expect(loading).toBeInViewport();
  const box = await boxOf(loading);
  const viewport = page.viewportSize()!;
  expect(Math.abs(box.x - (viewport.width - box.x - box.width)), 'loading snackbar centered').toBeLessThanOrEqual(1.5);
  expect(box.width).toBeLessThan(viewport.width - 16);
  await page.unroute('**/api/auth/login');

  await expect(page.getByText('Das Passwort stimmt nicht.')).toBeInViewport();
  await expect(password).toHaveValue('');

  await password.fill(DASHBOARD_PASSWORD);
  await page.getByRole('button', { name: 'Anmelden', exact: true }).click();
  await expect(page).toHaveURL(/\/$/);
});

test('signs out through the side menu', async ({ page }) => {
  await page.goto('/auth/sign-in');
  await page.getByRole('button', { name: 'Mit Geräte-Passwort anmelden' }).click();
  await page.getByRole('textbox', { name: 'Passwort', exact: true }).fill(DASHBOARD_PASSWORD);
  await page.getByRole('button', { name: 'Anmelden', exact: true }).click();
  await expect(page).toHaveURL(/\/$/);

  await page.getByRole('button', { name: 'Menü öffnen' }).click();
  await page.getByRole('navigation', { name: 'Hauptmenü' }).getByRole('link', { name: 'Abmelden' }).click();
  await expect(page).toHaveURL(/\/auth\/sign-out$/);
  await page.getByRole('button', { name: 'Abmelden', exact: true }).click();

  await expect(page).toHaveURL(/\/auth\/sign-in/);
  await expect(page.getByText('Abgemeldet')).toBeVisible();
});
