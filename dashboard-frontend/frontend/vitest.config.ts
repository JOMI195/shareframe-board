import { defineConfig } from 'vitest/config';
import react from '@vitejs/plugin-react';
import path from 'path';

export default defineConfig({
  plugins: [react()],
  resolve: {
    alias: {
      '@': path.resolve(import.meta.dirname, 'src'),
      '@tests': path.resolve(import.meta.dirname, 'tests/support'),
    },
  },
  test: {
    environment: 'jsdom',
    globals: false,
    setupFiles: ['./tests/support/setup/vitest.setup.ts'],
    include: ['tests/{unit,integration}/**/*.test.{ts,tsx}'],
    exclude: ['node_modules/**', 'build/**'],
    restoreMocks: true,
    clearMocks: true,
    // Rendering a whole route tree under coverage instrumentation outruns the 5s default.
    testTimeout: 15_000,
    // The env files live outside this folder and are not meant for tests.
    env: {
      TZ: 'UTC',
      VITE_APP_PRODUCTION: 'False',
      VITE_DEV_API_BASE_URL: 'http://localhost:8080',
    },
    coverage: {
      provider: 'v8',
      reporter: ['text', 'html'],
      include: ['src/**'],
      exclude: ['src/types/**', 'src/assets/**', 'src/**/*.d.ts'],
      // The measured baseline, rounded down. Raise it, never lower it.
      thresholds: { statements: 90, branches: 78, functions: 89, lines: 91 },
    },
  },
});
