import { expect, signIn, test } from '../support/fixtures';

test('protected pages redirect to sign-in', async ({ page }) => {
  await page.goto('/network/');
  await expect(page).toHaveURL(/\/auth\/sign-in/);
});

test('a wrong password is rejected', async ({ page }) => {
  await page.goto('/auth/sign-in');
  await page.getByRole('button', { name: 'Mit Passwort anmelden' }).click();
  await page.getByLabel(/Passwort/).fill('wrong-password');
  await page.getByRole('button', { name: 'Anmelden', exact: true }).click();
  await expect(page.getByText('Passwort ungültig')).toBeVisible();
  await expect(page).toHaveURL(/\/auth\/sign-in/);
});

test('an OTP is verified against the server', async ({ page }) => {
  await page.goto('/auth/sign-in');
  await page.getByLabel(/OTP/).fill('123456');
  await page.getByRole('button', { name: 'Anmelden', exact: true }).click();
  await expect(page).toHaveURL(/\/$/);
  await expect(page.getByText('Bilderwiedergabe').first()).toBeVisible();
});

test('an invalid OTP is rejected by the server', async ({ page }) => {
  await page.goto('/auth/sign-in');
  await page.getByLabel(/OTP/).fill('000000');
  await page.getByRole('button', { name: 'Anmelden', exact: true }).click();
  await expect(page.getByText('OTP invalid')).toBeVisible();
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
