import { isCleared, showsDefault, showsImage, waitForPanel } from '../support/board';
import { expect, signIn, skipActionLock, test } from '../support/fixtures';
import { boxOf } from '../support/layout';

test.afterEach(async ({ board }) => {
  await board.post('/api/frame/slideshow/order', { mode: 'chronological', custom_order: [] });
});

test('skips a photo and shows the outcome inside the screen', async ({ page, server }) => {
  const a = await server.sendPicture('left-black.jpg');
  await waitForPanel(showsImage(a), `picture ${a} on panel`);
  const b = await server.sendPicture('top-black.jpg');
  await signIn(page);

  await page.getByRole('button', { name: 'Foto überspringen' }).click();

  const outcome = page.getByRole('alert').filter({ hasText: 'Foto übersprungen' });
  await expect(outcome).toBeInViewport({ ratio: 1 });
  await waitForPanel(showsImage(b), `skip to ${b}`);
});

test('stops and starts the slideshow on a phone', async ({ page }) => {
  await signIn(page);
  await page.getByRole('button', { name: 'Stoppen' }).click();
  await expect(page.getByText('Diashow gestoppt')).toBeVisible();
  await waitForPanel(isCleared, 'panel cleared');

  await skipActionLock(page);
  await page.getByRole('button', { name: 'Starten' }).click();
  await expect(page.getByText('Diashow gestartet')).toBeVisible();
  await waitForPanel(showsDefault, 'default image after start');
});

test('the floating buttons cover neither the last content nor the outcome', async ({ page, server }) => {
  await server.sendPicture('left-black.jpg', { sender: 'anna' });
  await server.sendPicture('top-black.jpg', { sender: 'ben' });
  await signIn(page);
  await page.goto('/slideshow/');
  await page.getByRole('button', { name: 'Eigene Reihenfolge' }).click();

  const save = page.getByRole('button', { name: 'Reihenfolge speichern' });
  const count = page.getByText('2 Fotos auf dem Bilderrahmen');
  // The tiles grow once their photos load, so the end of the page moves for a moment.
  await expect.poll(async () => {
    await page.evaluate(() => window.scrollTo(0, document.documentElement.scrollHeight));
    const text = await boxOf(count);
    return (await boxOf(save)).y - (text.y + text.height);
  }, { message: 'gap between the last content and the floating buttons' }).toBeGreaterThanOrEqual(0);

  await page.getByRole('button', { name: 'Foto von ben nach vorne verschieben' }).click();
  await save.click();
  const outcome = page.getByRole('alert').filter({ hasText: 'Eigene Reihenfolge ab dem nächsten Foto' });
  await expect(outcome).toBeInViewport({ ratio: 1 });
});

test('an error stays until it is closed', async ({ page }) => {
  await signIn(page);
  await page.goto('/network/');
  await page.getByRole('button', { name: '„E2E-WiFi“ entfernen' }).click();
  await page.getByRole('dialog').getByRole('button', { name: 'Entfernen' }).click();

  const error = page.getByRole('alert').filter({ hasText: 'WLAN konnte nicht entfernt werden' });
  await expect(error).toBeInViewport({ ratio: 1 });
  await page.clock.fastForward('00:30');
  await expect(error).toBeVisible();

  await error.getByRole('button', { name: 'Schließen' }).click();
  await expect(error).toBeHidden();
});
