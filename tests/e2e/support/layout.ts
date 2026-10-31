import type { Locator, Page } from '@playwright/test';
import { expect } from './fixtures';

export const expectNoHorizontalScroll = async (page: Page) => {
  const { scrollWidth, innerWidth } = await page.evaluate(() => ({
    scrollWidth: document.scrollingElement!.scrollWidth,
    innerWidth: window.innerWidth,
  }));
  expect(scrollWidth).toBeLessThanOrEqual(innerWidth);
};

export const boxOf = async (locator: Locator) => {
  const box = await locator.boundingBox();
  expect(box, 'element has no layout box').not.toBeNull();
  return box!;
};
