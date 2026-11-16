import type { Locator, Page } from '@playwright/test';
import { expect } from './fixtures';

export const expectNoHorizontalScroll = async (page: Page) => {
  const { scrollWidth, innerWidth } = await page.evaluate(() => ({
    scrollWidth: document.scrollingElement!.scrollWidth,
    innerWidth: window.innerWidth,
  }));
  expect(scrollWidth).toBeLessThanOrEqual(innerWidth);
};

export const expectNoClippedCards = async (page: Page) => {
  const clipped = await page.locator('#main .MuiCard-root').evaluateAll((cards) =>
    cards
      .filter((c) => [c, ...c.children].some((e) =>
        e.scrollHeight > e.clientHeight + 1 && !['auto', 'scroll'].includes(getComputedStyle(e).overflowY)))
      .map((c) => c.querySelector('h2')?.textContent ?? c.textContent?.slice(0, 40)));
  expect(clipped, 'cards cut off at the bottom').toEqual([]);
};

export const expectNoEmptyCardSpace = async (page: Page) => {
  const stretched = await page.locator('#main .MuiCard-root').evaluateAll((cards) =>
    cards
      .filter((c) => {
        const sections = c.querySelector('.MuiCardContent-root')?.lastElementChild;
        if (!sections || getComputedStyle(sections).flexGrow !== '1') return false;
        const used = [...sections.children].reduce((sum, e) => {
          const s = getComputedStyle(e);
          return sum + e.getBoundingClientRect().height + parseFloat(s.marginTop) + parseFloat(s.marginBottom);
        }, 0);
        return sections.getBoundingClientRect().height - used > 8;
      })
      .map((c) => c.querySelector('h2')?.textContent ?? c.textContent?.slice(0, 40)));
  expect(stretched, 'cards stretched with empty space').toEqual([]);
};

export const boxOf = async (locator: Locator) => {
  const box = await locator.boundingBox();
  expect(box, 'element has no layout box').not.toBeNull();
  return box!;
};
