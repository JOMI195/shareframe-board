import path from 'node:path';
import { FIXTURES } from '../support/board';
import { expect, signIn, test } from '../support/fixtures';
import { boxOf, expectNoHorizontalScroll } from '../support/layout';

test('adds a default image from a phone in the full-screen editor', async ({ page, board }) => {
  await signIn(page);
  await page.goto('/default-images/');
  await expect(page.getByText('3 von höchstens 10 Standardbildern')).toBeVisible();
  await expectNoHorizontalScroll(page);

  await page.getByRole('button', { name: 'Bild hinzufügen' }).click();
  const dialog = page.getByRole('dialog', { name: 'Standardbild hinzufügen' });
  const viewport = page.viewportSize()!;
  expect((await boxOf(dialog)).width).toBe(viewport.width);
  await dialog.locator('xpath=..').locator('input[type="file"]').setInputFiles(path.join(FIXTURES, 'gradient.jpg'));
  await expect(dialog.getByRole('tab', { name: 'Zuschnitt' })).toBeVisible();
  await expectNoHorizontalScroll(page);

  await dialog.getByRole('button', { name: 'Weiter zur Vorschau' }).click();
  await dialog.getByRole('button', { name: 'E-Paper' }).click();
  await expect(dialog.getByRole('img', { name: /Vorschau von gradient\.jpg/ })).toBeVisible();
  await dialog.getByRole('button', { name: 'Hochladen' }).click();

  await expect(dialog).toBeHidden();
  await expect(page.getByText('4 von höchstens 10 Standardbildern')).toBeVisible();
  expect(await board.defaultImageNames()).toHaveLength(4);
});
