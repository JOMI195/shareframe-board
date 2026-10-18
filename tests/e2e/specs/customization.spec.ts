import { attachPanel, Frame, showsImage, waitForPanel } from '../support/board';
import { expect, signIn, test } from '../support/fixtures';

// The customization page ("Anpassung"): how pictures are drawn on the panel.

// Top-right corner of the visible area, where panel_badge::area puts the sender badge.
const badge = [418, 44, 718, 88] as const;

test('the sender name shows once turned on and survives a restart', async ({ page, board, server }, testInfo) => {
  const id = await server.sendPicture('left-black.jpg', { sender: 'marta' });
  await waitForPanel(showsImage(id), `picture ${id} on panel`);
  expect((await Frame.read()).blackRatio(...badge), 'badge while off').toBeLessThan(0.01);

  await signIn(page);
  await page.goto('/customization/');
  await page.getByLabel('Name des Absenders anzeigen').check();
  await expect(page.getByText('Der Name des Absenders wird ab dem nächsten Bildwechsel angezeigt')).toBeVisible();

  await board.restartService('display');
  expect(await board.get('/api/frame/display/customization')).toEqual({ show_sender: true });
  await expect.poll(async () => (await Frame.read()).blackRatio(...badge), { message: 'sender badge', timeout: 20_000 })
    .toBeGreaterThan(0.03);
  await attachPanel(testInfo, 'sender-badge');
});
