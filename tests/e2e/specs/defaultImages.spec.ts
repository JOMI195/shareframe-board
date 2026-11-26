import path from 'node:path';
import type { Page } from '@playwright/test';
import { FIXTURES, readPanel, type BoardApi } from '../support/board';
import { expect, signIn, test } from '../support/fixtures';

// The default images page: what the frame shows while nobody has sent it a picture.

const uploadThroughTheEditor = async (page: Page, fixture: string) => {
  await page.getByRole('button', { name: 'Bild hinzufügen' }).first().click();
  const dialog = page.getByRole('dialog', { name: 'Standardbild hinzufügen' });
  await dialog.locator('xpath=..').locator('input[type="file"]').setInputFiles(path.join(FIXTURES, fixture));
  await dialog.getByRole('button', { name: 'Weiter zur Vorschau' }).click();
  await dialog.getByRole('button', { name: 'Hochladen' }).click();
  return dialog;
};

const playsOnThePanel = async (board: BoardApi, name: string) =>
  expect.poll(async () => {
    if ((await readPanel())?.source.endsWith(`/${name}`)) return true;
    await board.skip();
    await new Promise((r) => setTimeout(r, 1500));
    return false;
  }, { message: `${name} on the panel`, timeout: 40_000 }).toBe(true);

test('an image added in the editor joins the set and plays on the panel', async ({ page, board }) => {
  const before = await board.defaultImageNames();
  await signIn(page);
  await page.goto('/default-images/');
  await expect(page.getByText('3 von höchstens 10 Standardbildern')).toBeVisible();

  const dialog = await uploadThroughTheEditor(page, 'left-black.jpg');

  await expect(dialog).toBeHidden();
  await expect(page.getByText('4 von höchstens 10 Standardbildern')).toBeVisible();
  const added = (await board.defaultImageNames()).find((name) => !before.includes(name));
  expect(added).toMatch(/^[0-9a-f]{64}\.jpg$/);
  await playsOnThePanel(board, added!);
});

test('an image that looks like one in the set is refused', async ({ page, board }) => {
  await board.uploadDefaultImage('left-black.jpg');
  await signIn(page);
  await page.goto('/default-images/');

  const dialog = await uploadThroughTheEditor(page, 'left-black.jpg');

  await expect(page.getByText(/schon ein Standardbild|fast genauso aus wie ein vorhandenes Standardbild/)).toBeVisible();
  await expect(dialog.getByRole('button', { name: 'Hochladen' })).toBeVisible();
  expect(await board.defaultImageNames()).toHaveLength(4);
});

test('the last three stay', async ({ page, board }) => {
  const bundled = await board.defaultImageNames();
  await board.uploadDefaultImage('left-black.jpg');
  await signIn(page);
  await page.goto('/default-images/');

  await page.getByRole('listitem', { name: 'Standardbild 4', exact: true }).click();
  await page.getByRole('dialog', { name: 'Standardbild', exact: true }).getByRole('button', { name: 'Entfernen' }).click();
  await page.getByRole('dialog', { name: 'Standardbild entfernen?' }).getByRole('button', { name: 'Entfernen' }).click();

  await expect(page.getByText('3 von höchstens 10 Standardbildern')).toBeVisible();
  await page.getByRole('listitem', { name: 'Standardbild 1', exact: true }).click();
  await expect(page.getByRole('dialog', { name: 'Standardbild', exact: true }).getByRole('button', { name: 'Entfernen' })).toBeDisabled();
  expect((await board.postRaw('/api/frame/default-images/delete', { name: bundled[0] })).status()).toBe(409);
  expect(await board.defaultImageNames()).toEqual(bundled);
});
