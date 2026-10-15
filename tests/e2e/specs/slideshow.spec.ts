import { showsImage, waitForPanel } from '../support/board';
import { expect, signIn, test } from '../support/fixtures';

// The slideshow page: the pictures on the frame and the order the slideshow plays them in.

test.afterEach(async ({ board }) => {
  await board.post('/api/frame/slideshow/order', { mode: 'chronological', custom_order: [] });
});

test('lists the pictures on the frame with their details', async ({ page, server }) => {
  await server.sendPicture('left-black.jpg', { sender: 'anna' });
  await server.sendPicture('top-black.jpg', { sender: 'ben' });
  await signIn(page);
  await page.goto('/slideshow/');

  await expect(page.getByText('2 Bilder auf dem Bilderrahmen')).toBeVisible();
  await page.getByRole('listitem', { name: /Bild von ben/ }).click();
  const dialog = page.getByRole('dialog');
  await expect(dialog.getByText('Position 2 von 2')).toBeVisible();
  await expect(dialog.getByText(/^\d+ × \d+ px$/)).toBeVisible();
  await expect(dialog.getByRole('img', { name: 'Bild von ben' })).toBeVisible();
});

test('a hand-picked order decides which picture the panel shows next', async ({ page, board, server }) => {
  const a = await server.sendPicture('left-black.jpg', { sender: 'anna' });
  await waitForPanel(showsImage(a), `picture ${a} on panel`);
  const b = await server.sendPicture('top-black.jpg', { sender: 'ben' });
  const c = await server.sendPicture('gradient.jpg', { sender: 'carla' });

  await signIn(page);
  await page.goto('/slideshow/');
  await page.getByRole('button', { name: 'Eigene Reihenfolge' }).click();
  await page.getByRole('button', { name: 'Bild von carla nach vorne verschieben' }).click();
  await page.getByRole('button', { name: 'Reihenfolge speichern' }).click();
  await expect(page.getByText('Eigene Reihenfolge ab dem nächsten Bild')).toBeVisible();

  await board.skip();
  await waitForPanel(showsImage(c), `custom order: ${c} after ${a}`);
  await board.skip();
  await waitForPanel(showsImage(b), `custom order: ${b} after ${c}`);
  expect((await board.get('/api/frame/slideshow/images')).custom_order).toEqual([a, c, b]);
});

test('newest first jumps from the oldest picture to the latest', async ({ page, board, server }) => {
  const a = await server.sendPicture('left-black.jpg');
  await waitForPanel(showsImage(a), `picture ${a} on panel`);
  await server.sendPicture('top-black.jpg');
  const c = await server.sendPicture('gradient.jpg');

  await signIn(page);
  await page.goto('/slideshow/');
  await page.getByRole('button', { name: 'Neueste zuerst' }).click();
  await expect(page.getByText('Neueste zuerst ab dem nächsten Bild')).toBeVisible();

  await board.skip();
  await waitForPanel(showsImage(c), `newest first: ${c} after ${a}`);
});
