import { test as base, expect, type Page } from '@playwright/test';
import { BoardApi, DASHBOARD_PASSWORD, FakeServer, readPanel, resetBoardState, showsDefault, waitForPanel } from './board';

type Fixtures = {
  server: FakeServer;
  board: BoardApi;
  cleanBoard: void;
  contentPolicy: void;
};

export const test = base.extend<Fixtures>({
  server: async ({ request }, use) => {
    await use(new FakeServer(request));
  },

  board: async ({ playwright, baseURL }, use) => {
    const request = await playwright.request.newContext({ baseURL });
    const board = new BoardApi(request);
    await board.login();
    await use(board);
    await request.dispose();
  },

  // Every test starts on a connected board running the slideshow without user pictures.
  cleanBoard: [
    async ({ server, board }, use) => {
      await resetBoardState();
      await server.reset();
      // The migrated database holds the picture from 1 to 6, so tests at night would see no changes.
      await board.post('/api/frame/slideshow/night-mode', { enabled: false, start_hour: 1, end_hour: 6, interval_seconds: 3600, style: 'slower' });
      await board.post('/api/frame/display/customization', { show_sender: false });
      await expect.poll(async () => (await board.slideshowStatus()).image_count, { message: 'user images cleared' }).toBe(0);
      if (!(await board.slideshowStatus()).active) await board.post('/api/frame/slideshow', { action: 'start' });
      // nng PUB/SUB drops the removal event while a restarted service reconnects,
      // which leaves a deleted picture on the panel until the next change.
      const panel = await readPanel();
      if (panel && !showsDefault(panel)) await board.skip();
      await waitForPanel(showsDefault, 'board back on default images');
      await use();
    },
    { auto: true },
  ],

  // The board's Content-Security-Policy must not block anything the dashboard loads.
  contentPolicy: [
    async ({ page }, use) => {
      const blocked: string[] = [];
      page.on('console', (m) => { if (m.type() === 'error' && /Content Security Policy/.test(m.text())) blocked.push(m.text()); });
      await use();
      expect(blocked, 'blocked by the Content-Security-Policy').toEqual([]);
    },
    { auto: true },
  ],
});

export { expect };

/** Signs in through the UI and waits for the first page. */
export async function signIn(page: Page, password = DASHBOARD_PASSWORD) {
  await page.clock.install();
  await page.goto('/auth/sign-in');
  // The page starts on the one-time code (online) or the setup code (hotspot); both offer the password.
  const choosePassword = page.getByRole('button', { name: 'Mit Geräte-Passwort anmelden' });
  const passwordBox = page.getByRole('textbox', { name: 'Passwort', exact: true });
  await expect(choosePassword.or(passwordBox).first()).toBeVisible();
  if (await choosePassword.isVisible()) await choosePassword.click();
  await passwordBox.fill(password);
  await page.getByRole('button', { name: 'Anmelden', exact: true }).click();
  // Without Wi-Fi the dashboard opens on the network page instead of home.
  const home = page.getByRole('heading', { level: 2, name: 'Diashow' });
  const network = page.getByRole('heading', { level: 1, name: 'Netzwerk' });
  await expect(home.or(network).first()).toBeVisible();
}

/** A dashboard card by its exact title. */
export function card(page: Page, title: string) {
  return page.locator('.MuiCard-root').filter({ has: page.getByRole('heading', { name: title, exact: true }) });
}

/** Collects every failed dashboard API call the page makes from now on. */
export function trackApiErrors(page: Page) {
  const failures: string[] = [];
  page.on('response', (res) => {
    if (res.url().includes('/api/') && res.status() >= 400) failures.push(`${res.status()} ${new URL(res.url()).pathname}`);
  });
  // A navigation aborts what the page left in flight; that is not a failing call.
  page.on('requestfailed', (req) => {
    if (req.url().includes('/api/') && req.failure()?.errorText !== 'net::ERR_ABORTED')
      failures.push(`failed ${new URL(req.url()).pathname}`);
  });
  return failures;
}
