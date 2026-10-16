import { attachPanel, isCleared, readPanel, showsDefault, showsImage, waitForPanel } from '../support/board';
import { card, expect, signIn, skipActionLock, test } from '../support/fixtures';

// The home page ("Steuerung"): slideshow status, controls and settings.

test.afterEach(async ({ board }) => {
  await board.post('/api/frame/slideshow/interval', { interval_seconds: 900 });
  await board.post('/api/frame/slideshow/night-mode', { enabled: false, start_hour: 1, end_hour: 6, interval_seconds: 3600, blank: false });
});

test('the status card shows what the board reports', async ({ page, server }) => {
  await server.sendPicture('left-black.jpg');
  await server.sendPicture('top-black.jpg');
  await signIn(page);
  await expect(page.getByText('Wird ausgeführt')).toBeVisible();
  await expect(page.getByText('2 Bilder')).toBeVisible();
  await expect(page.getByText(/^1[45]:\d\d$/)).toBeVisible();
});

test('the now-showing card shows the picture on the panel', async ({ page, server }) => {
  const id = await server.sendPicture('left-black.jpg', { sender: 'marta' });
  await waitForPanel(showsImage(id), `picture ${id} on panel`);

  const file = page.waitForResponse((res) => res.url().includes(`/api/frame/images/file?id=${id}`) && res.ok());
  await signIn(page);
  const preview = card(page, 'Aktuell angezeigt');
  await expect(preview.getByText('Bild von marta')).toBeVisible();
  await expect(preview.getByRole('img', { name: 'Aktuell angezeigtes Bild' })).toBeVisible();
  expect((await file).headers()['content-type']).toBe('image/jpeg');
});

test('skipping changes the panel', async ({ page, server }, testInfo) => {
  const a = await server.sendPicture('left-black.jpg');
  await waitForPanel(showsImage(a), `picture ${a} on panel`);
  const b = await server.sendPicture('top-black.jpg');

  await signIn(page);
  await expect(page.getByText('2 Bilder')).toBeVisible();
  await page.getByRole('button', { name: 'Bild überspringen' }).click();

  await expect(page.getByText('Aktuelles Bild erfolgreich übersprungen')).toBeVisible();
  await waitForPanel(showsImage(b), `skip to ${b}`);
  await attachPanel(testInfo, 'after-skip');
});

test('stopping clears the panel and starting shows pictures again', async ({ page, board }) => {
  await signIn(page);
  await page.getByRole('button', { name: 'Stoppen' }).click();

  await expect(page.getByText('Bilderwiedergabe erfolgreich gestoppt')).toBeVisible();
  await waitForPanel(isCleared, 'panel cleared');
  expect((await board.slideshowStatus()).active).toBe(false);
  await expect(page.getByText('Gestoppt', { exact: true })).toBeVisible();

  await skipActionLock(page);
  await page.getByRole('button', { name: 'Starten' }).click();
  await expect(page.getByText('Bilderwiedergabe erfolgreich gestartet')).toBeVisible();
  await waitForPanel(showsDefault, 'default image after start');
});

test('clearing the screen of a stopped slideshow blanks the panel', async ({ page, board }) => {
  await board.post('/api/frame/slideshow', { action: 'stop' });
  await signIn(page);
  await page.getByRole('button', { name: 'Bildschirm leeren' }).click();

  await expect(page.getByText('Bildschirm erfolgreich geleert')).toBeVisible();
  await waitForPanel(isCleared, 'panel cleared');
});

test('a stopped slideshow ignores skip requests', async ({ board }) => {
  await board.post('/api/frame/slideshow', { action: 'stop' });
  await board.post('/api/frame/clear');
  const cleared = await waitForPanel(isCleared, 'panel cleared');

  await board.skip();
  await new Promise((r) => setTimeout(r, 2000));
  expect((await readPanel())?.seq).toBe(cleared.seq);
});

test('the interval reaches the display and survives a restart', async ({ page, board }) => {
  await signIn(page);
  await page.getByLabel('Intervall', { exact: true }).fill('30');
  await page.getByRole('button', { name: 'Intervall speichern' }).click();
  await expect(page.getByText('Intervall der Bilderwiedergabe erfolgreich auf 30 Minuten aktualisiert')).toBeVisible();

  await expect.poll(async () => (await board.slideshowStatus()).interval_seconds).toBe(1800);
  await board.restartService('display');
  expect((await board.slideshowStatus()).interval_seconds).toBe(1800);
});

test('night mode reaches the display and survives a restart', async ({ page, board }) => {
  await signIn(page);
  await page.getByLabel('Nachtmodus aktivieren').check();
  await page.getByLabel('Bildschirm nachts pausieren').check();
  await page.getByRole('button', { name: 'Nachtmodus speichern' }).click();
  await expect(page.getByText(/Nachtmodus aktiv von/)).toBeVisible();

  await expect.poll(async () => (await board.slideshowStatus()).night_mode.enabled).toBe(true);
  await board.restartService('display');
  expect((await board.slideshowStatus()).night_mode).toMatchObject({ enabled: true, blank: true });
});

test('a stopped slideshow stays stopped across a display restart', async ({ board }) => {
  await board.post('/api/frame/slideshow', { action: 'stop' });
  await board.restartService('display');
  expect((await board.slideshowStatus()).active).toBe(false);
});

test('the board rejects unauthenticated and out-of-range changes', async ({ request, board }) => {
  expect((await request.post('/api/frame/slideshow/interval', { data: { interval_seconds: 600 } })).status()).toBe(401);
  for (const interval_seconds of [0, 179, 86401]) {
    const res = await board.postRaw('/api/frame/slideshow/interval', { interval_seconds });
    expect(res.status(), String(interval_seconds)).toBe(400);
  }
});
