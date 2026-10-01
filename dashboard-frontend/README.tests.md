# Testing

The dashboard frontend has a Vitest suite that runs without the board or Docker.

```bash
cd frontend
npm run check          # lint + tsc + unit/integration — run before committing
npm run test           # unit + integration
npm run test:coverage  # same, with the coverage thresholds enforced
```

```
frontend/tests/
├─ unit/          *.test.ts    pure logic, mirrors src/
├─ integration/   *.test.tsx   components + real store + MSW; flows/ uses the real routes
└─ support/       NO TESTS LIVE HERE
```

**Which tier?** No DOM and no network → unit. MSW can answer it → integration.
There is no e2e tier yet: the C++ dashboard server has no test stack.

## Support

- `mocks/handlers/` is the default world: signed in, online, slideshow running, all
  services healthy, no update. Every answer uses the board's `{ success, message, data }`
  envelope (`ok()` / `fail()`).
- `mocks/scenarios.ts` layers data or failures on top: `server.use(...apMode())`,
  `failsWith('post', api.getLoginUrl(), 401, 'OTP invalid')`, `spyOnRequests(...)`.
- The slideshow toggle polls until the board flips, so the default world keeps that
  state; `setBoard({ slideshowActive: false })` starts a test with it stopped.
- `renderRoute(path)` mounts the real route table, `renderWithProviders(ui)` a single
  component. Both use an unpersisted store from `src/store/setupStore.ts`.
- `RouterContext` polls the connection mode on mount and overwrites preloaded
  `connectionMode` state; use `withConnectionMode()` instead.
- The home page locks its actions for 20s after mount and 3 minutes after every
  action. `useBoardClock()` + `advance(ms)` from `helpers/time.ts` drive that.

## Rules

- MSW runs with `onUnhandledRequest: 'error'`; handlers are built from the app's own
  URL builders in `src/assets/endpoints/api/frame.ts`, so a renamed path breaks them
  instead of silently passing. Never call `fetchWithTimeout` with a literal path.
- jsdom gaps are patched only in `vitest.config.ts` and `tests/support/setup/`.
- Comments say why a test looks unusual, never what the code does. A test that pins a
  known bug rather than the intended behaviour says so — `grep -rn "Known bug"`.
