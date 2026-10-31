import { DASHBOARD_PASSWORD } from '../support/board';
import { expect, signIn, test } from '../support/fixtures';

test('protected pages redirect to sign-in', async ({ page }) => {
  await page.goto('/network/');
  await expect(page).toHaveURL(/\/auth\/sign-in/);
});

test('a wrong password is rejected', async ({ page }) => {
  await page.goto('/auth/sign-in');
  await page.getByRole('button', { name: 'Mit Passwort anmelden' }).click();
  await page.getByRole('textbox', { name: 'Passwort', exact: true }).fill('wrong-password');
  await page.getByRole('button', { name: 'Anmelden', exact: true }).click();
  await expect(page.getByText('Das Passwort stimmt nicht.')).toBeVisible();
  await expect(page).toHaveURL(/\/auth\/sign-in/);
});

test('a valid password signs in', async ({ page }) => {
  await page.goto('/auth/sign-in');
  await page.getByRole('button', { name: 'Mit Passwort anmelden' }).click();
  await page.getByRole('textbox', { name: 'Passwort', exact: true }).fill(DASHBOARD_PASSWORD);
  await page.getByRole('button', { name: 'Anmelden', exact: true }).click();
  await expect(page).toHaveURL(/\/$/);
  await expect(page.getByRole('heading', { level: 2, name: 'Diashow' })).toBeVisible();
});

test('a one-time code is verified against the server', async ({ page }) => {
  await page.goto('/auth/sign-in');
  await page.getByRole('textbox', { name: 'Einmal-Code', exact: true }).fill('123456');
  await page.getByRole('button', { name: 'Anmelden', exact: true }).click();
  await expect(page).toHaveURL(/\/$/);
  await expect(page.getByRole('heading', { level: 2, name: 'Diashow' })).toBeVisible();
});

test('an invalid one-time code is rejected by the server', async ({ page }) => {
  await page.goto('/auth/sign-in');
  await page.getByRole('textbox', { name: 'Einmal-Code', exact: true }).fill('000000');
  await page.getByRole('button', { name: 'Anmelden', exact: true }).click();
  await expect(page.getByText('Der Einmal-Code stimmt nicht oder ist abgelaufen.')).toBeVisible();
});

test('signing out ends the session on the board', async ({ page }) => {
  await signIn(page);
  const session = (await page.context().cookies()).find((c) => c.name === 'session');
  expect(session).toBeDefined();

  await page.goto('/auth/sign-out');
  await page.getByRole('button', { name: 'Abmelden', exact: true }).click();
  await expect(page).toHaveURL(/\/auth\/sign-in/);

  const replay = await page.request.get('/api/frame/slideshow/status', { headers: { cookie: `session=${session!.value}` } });
  expect(replay.status()).toBe(401);
});
