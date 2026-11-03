import { DASHBOARD_PASSWORD } from '../support/board';
import { expect, test } from '../support/fixtures';
import { expectNoHorizontalScroll } from '../support/layout';

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
  await page.getByRole('button', { name: 'Mit Passwort anmelden' }).click();
  const password = page.getByRole('textbox', { name: 'Passwort', exact: true });
  await password.fill('wrong-password');
  await page.getByRole('button', { name: 'Anmelden', exact: true }).click();

  await expect(page.getByText('Das Passwort stimmt nicht.')).toBeInViewport();
  await expect(password).toHaveValue('');

  await password.fill(DASHBOARD_PASSWORD);
  await page.getByRole('button', { name: 'Anmelden', exact: true }).click();
  await expect(page).toHaveURL(/\/$/);
});

test('signs out through the side menu', async ({ page }) => {
  await page.goto('/auth/sign-in');
  await page.getByRole('button', { name: 'Mit Passwort anmelden' }).click();
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
