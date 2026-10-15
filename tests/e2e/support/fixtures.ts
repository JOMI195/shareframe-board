import { test as base, expect, type Page } from '@playwright/test';
import { BoardApi, DASHBOARD_PASSWORD, FakeServer, readPanel, resetBoardState, showsDefault, waitForPanel } from './board';

type Fixtures = {
  server: FakeServer;
  board: BoardApi;
  cleanBoard: void;
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
      await expect.poll(async () => (await board.slideshowStatus()).image_count, { message: 'user images cleared' }).toBe(0);
      if (!(await board.slideshowStatus()).active) await board.post('/api/frame/slideshow', { action: 'start' });
      // nng PUB/SUB drops the removal event while a restarted service reconnects,
      // which leaves a deleted picture on the panel until the next change.
      const panel = await readPanel();
      if (panel && !showsDefault(panel)) await board.post('/api/frame/slideshow/skip');
      await waitForPanel(showsDefault, 'board back on default images');
      await use();
    },
    { auto: true },
  ],
});

export { expect };

/** Signs in through the UI and waits until the home page's start-up lock is over. */
export async function signIn(page: Page, password = DASHBOARD_PASSWORD) {
  await page.clock.install();
  await page.goto('/auth/sign-in');
  await page.getByRole('button', { name: 'Mit Passwort anmelden' }).click();
  await page.getByLabel(/Passwort/).fill(password);
  await page.getByRole('button', { name: 'Anmelden', exact: true }).click();
  await expect(page.getByText('Bilderwiedergabe').first()).toBeVisible();
  await page.clock.fastForward('00:21');
}

/** Jumps over the 3-minute lock the UI puts on slideshow actions after each action. */
export async function skipActionLock(page: Page) {
  await page.clock.fastForward('03:05');
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
  page.on('requestfailed', (req) => {
    if (req.url().includes('/api/')) failures.push(`failed ${new URL(req.url()).pathname}`);
  });
  return failures;
}
