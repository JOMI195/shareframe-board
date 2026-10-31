import { expect, signIn, test } from '../support/fixtures';
import { expectNoHorizontalScroll } from '../support/layout';

const mainPages = [
  { name: 'Diashow', url: /\/slideshow\/$/ },
  { name: 'Anpassung', url: /\/customization\/$/ },
  { name: 'Netzwerk', url: /\/network\/$/ },
  { name: 'Updates', url: /\/updates\/$/ },
  { name: 'Steuerung', url: /\/$/ },
];

const adminPages = ['/services/', '/services/display', '/general/', '/display-health/', '/update-history/', '/logs/'];

test.describe('navigation on a phone', () => {
  test('reaches every main page through the side menu without sideways scrolling', async ({ page }) => {
    await signIn(page);
    await expectNoHorizontalScroll(page);

    for (const { name, url } of mainPages) {
      await test.step(name, async () => {
        const link = page.getByRole('navigation', { name: 'Hauptmenü' }).getByRole('link', { name, exact: true });
        await page.getByRole('button', { name: 'Menü öffnen' }).click();
        await expect(link).toBeInViewport();

        await link.click();

        await expect(page).toHaveURL(url);
        await expect(page).toHaveTitle(`${name} – ShareFrame`);
        await expect(link).not.toBeInViewport();
        await expectNoHorizontalScroll(page);
      });
    }
  });

  test('the back button closes the menu instead of leaving the page', async ({ page }) => {
    await signIn(page);
    await page.goto('/network/');
    await page.getByRole('button', { name: 'Menü öffnen' }).click();
    const menu = page.getByRole('navigation', { name: 'Hauptmenü' });
    await expect(menu).toBeInViewport();

    await page.goBack();

    await expect(menu).not.toBeInViewport();
    await expect(page).toHaveURL(/\/network\/$/);
  });

  test('a reload does not reopen the menu', async ({ page }) => {
    await signIn(page);
    await page.getByRole('button', { name: 'Menü öffnen' }).click();
    await expect(page.getByRole('navigation', { name: 'Hauptmenü' })).toBeInViewport();

    await page.reload();

    await expect(page.getByRole('button', { name: 'Menü öffnen' })).toBeVisible();
    await expect(page.getByRole('navigation', { name: 'Hauptmenü' })).not.toBeInViewport();
  });

  test('the admin pages fit the screen as well', async ({ page }) => {
    await signIn(page);
    for (const path of adminPages) {
      await test.step(path, async () => {
        await page.goto(path);
        await expect(page.locator('h1')).toBeAttached();
        await expectNoHorizontalScroll(page);
      });
    }
  });

  test('the skip link moves the focus to the page content', async ({ page }) => {
    await signIn(page);

    await page.keyboard.press('Tab');
    const skip = page.getByRole('link', { name: 'Zum Inhalt springen' });
    await expect(skip).toBeFocused();
    await expect(skip).toBeInViewport();

    await page.keyboard.press('Enter');
    await expect(page.locator('#main')).toBeFocused();
  });
});
