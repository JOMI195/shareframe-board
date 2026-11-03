import { defineConfig, devices } from '@playwright/test';

const shared = {
  locale: 'de-DE',
  timezoneId: 'Europe/Berlin',
};

export default defineConfig({
  testDir: './specs',
  // One board is shared by every test, so they must not overlap.
  workers: 1,
  fullyParallel: false,
  timeout: 60_000,
  expect: { timeout: 15_000 },
  reporter: [['list'], ['html', { open: 'never', outputFolder: 'playwright-report' }]],
  outputDir: 'test-results',
  use: {
    baseURL: process.env.E2E_BASE_URL ?? 'http://frontend',
    trace: 'retain-on-failure',
    screenshot: 'only-on-failure',
  },
  projects: [
    {
      name: 'desktop',
      use: { ...shared, viewport: { width: 1600, height: 900 } },
      testIgnore: '**/*.mobile.spec.ts',
    },
    // Owners mostly set the frame up from a phone, often on its hotspot.
    {
      name: 'mobile',
      use: { ...devices['Pixel 7'], ...shared },
      testMatch: '**/*.mobile.spec.ts',
      dependencies: ['desktop'],
    },
  ],
});
