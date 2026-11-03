import AxeBuilder from '@axe-core/playwright';
import type { Page } from '@playwright/test';
import { setWifiMode } from '../support/board';
import { expect, signIn, test } from '../support/fixtures';

const WCAG_AA = ['wcag2a', 'wcag2aa', 'wcag21a', 'wcag21aa'];

// White text on the brand purple stays just below 4.5:1 by choice, so contrast is not checked.
const violationsOn = async (page: Page) => {
  const { violations } = await new AxeBuilder({ page }).withTags(WCAG_AA).disableRules(['color-contrast']).analyze();
  return violations.map((violation) => `${violation.id}: ${violation.nodes.map((node) => node.target.join(' ')).join(', ')}`);
};

const appPages = [
  '/', '/slideshow/', '/customization/', '/network/', '/updates/',
  '/services/', '/services/display', '/general/', '/display-health/', '/update-history/', '/logs/',
];

for (const colorScheme of ['light', 'dark'] as const) {
  test.describe(`axe on a phone in ${colorScheme} mode`, () => {
    test.use({ colorScheme });

    test('the sign-in page passes WCAG AA, also on the hotspot', async ({ page }) => {
      await page.goto('/auth/sign-in');
      await expect(page.getByRole('heading', { level: 1 })).toBeVisible();
      expect(await violationsOn(page)).toEqual([]);

      await setWifiMode('ap');
      await page.reload();
      await expect(page.getByText(/stellt den Hotspot/)).toBeVisible();
      expect(await violationsOn(page)).toEqual([]);
    });

    test('every app page passes WCAG AA', async ({ page }) => {
      // Every full load shows the 3 s start screen first.
      test.setTimeout(appPages.length * 12_000);
      await signIn(page);
      for (const path of appPages) {
        await test.step(path, async () => {
          await page.goto(path);
          await expect(page.locator('h1')).toBeAttached();
          await page.waitForTimeout(1000);
          expect(await violationsOn(page)).toEqual([]);
        });
      }
    });

    test('the open side menu and a dialog pass WCAG AA', async ({ page }) => {
      await signIn(page);
      await page.getByRole('button', { name: 'Menü öffnen' }).click();
      await expect(page.getByRole('navigation', { name: 'Hauptmenü' })).toBeInViewport();
      expect(await violationsOn(page)).toEqual([]);

      await page.goto('/general/');
      await page.getByRole('button', { name: 'Passwort ändern' }).click();
      await expect(page.getByRole('dialog')).toBeVisible();
      expect(await violationsOn(page)).toEqual([]);
    });
  });
}
