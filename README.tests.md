# Testing

Three suites: C++ (GoogleTest), dashboard frontend (Vitest) and e2e (Playwright against
the real board processes). `./test.sh` in the repo root runs all of them.

```bash
./test.sh unit          # C++, no I/O, seconds
./test.sh integration   # C++ against SQLite, nng sockets, temp dirs
./test.sh cpp           # unit + integration — run before committing C++ changes
./test.sh asan          # all C++ tests under AddressSanitizer + UBSan
./test.sh coverage      # all C++ tests + gcovr HTML report
./test.sh frontend      # lint + tsc + Vitest
./test.sh e2e           # builds the stack, runs the specs, destroys it
./test.sh all           # cpp + frontend + e2e
```

- `--docker` runs any C++ command in `tests/docker` (Debian, GCC 14). Output lands in
  `build-docker/`, owned by you. Use it for `asan` and `coverage` when the host lacks
  `libasan`/`libubsan` or `gcovr`.
- Extra arguments go to ctest or Playwright: `./test.sh unit -R VersionCompare`,
  `./test.sh e2e --grep skip`.

## C++

```bash
cmake --preset test && cmake --build --preset test     # what ./test.sh does
ctest --test-dir build-test -R <regex>                 # single tests
build-test/tests/shareframe-unit-tests --gtest_filter=Suite.Name
SHAREFRAME_TEST_LOG=debug ctest --test-dir build-test  # logs are silent by default
```

```
tests/
├─ CMakeLists.txt   two executables: shareframe-unit-tests, shareframe-integration-tests
├─ unit/            *Test.cpp   mirrors src/
├─ integration/     *Test.cpp   mirrors src/
│  └─ contract/     one service's IPC client against another's handler, over real nng
└─ support/         NO TESTS LIVE HERE
```

**Which tier?** Decided by the boundary a test crosses. No disk, database, socket or
thread → unit. Anything else, including a real `EventBus` or task thread → integration.
A message that crosses a process boundary → contract: drive it from the sending side
(e.g. `FrameHandlers`) and assert what arrives on the receiving bus, so a renamed field
on either side fails.

`support/` has `TempDir`, `TestDb`, `ImageFixture`, `Recorder<Topic>` (captures bus
messages), `waitFor`, `FakeHttpServer` / `FakeWsServer` (the Shareframe server),
`PathShim` (fake board commands on `PATH`), `storeValidToken` and `httpRequest`/`result`
for dashboard handlers.

- Tests are off by default (`SHAREFRAME_BUILD_TESTS`); the Buildroot build never sees them.
  Presets: `test`, `test-asan`, `test-coverage`. CLion picks them up.
- Tests link the per-app libraries (`shareframe-<app>-lib`), so everything except `main.cpp`
  is reachable. New sources go into the matching library, not the executable.
- **SQLite is never mocked.** `TestDb` is a temp file with the real `migrations/` applied.
- Each test owns its `TempDir`, its sockets (`ipc://<TempDir>/x.sock`) and its database.
- Threads are awaited with `waitFor(pred)`, never a fixed sleep. A sleep is only allowed to
  prove that something did *not* happen, and then it is short.
- Subscribe to the `EventBus` (and `start()` tasks) before `bus.start()`; `stop()` tasks
  before they go out of scope — `Task`'s destructor runs after the subclass is gone.
- Time-dependent logic gets a pure function that takes the time as input
  (`night::secsUntilEnd`, `daily::secsUntilHour`), instead of a test that depends on the
  wall clock.
- Logic that lives in an app's `main.cpp` cannot be tested; move it into the app's
  library first (`DisplayIpc`, `UpdateIpc`).
- The board's server and its shell commands are faked at the process boundary
  (`FakeHttpServer`, `PathShim`), never inside the code under test.

## Frontend

See [`dashboard-frontend/README.tests.md`](dashboard-frontend/README.tests.md). MSW
answers in the board's `{ success, message, data }` envelope; e2e is where the real
board's messages are checked against what the mocks assume.

## E2E

```
tests/e2e/
├─ board/         image with all five services, mock display, supervisor, command shims
├─ fake-server/   stand-in for the Shareframe server, driven through /__test/*
├─ frontend/      production dashboard build behind nginx, /api proxied to the board
├─ specs/         *.spec.ts — one per dashboard page, plus server-traffic.spec.ts
└─ support/       NO TESTS LIVE HERE — fixtures, board API, fake server, panel helpers
```

**Which tier?** Only true across processes (dashboard → IPC → display, server → websocket
→ display), against the real HTTP API, on the board's side effects or on the rendered
frame → e2e. What the frontend does with a mocked answer belongs in its Vitest suite.
Prefer a step in an existing spec over a new one; the stack takes about a minute to come up.

- **One spec per page** (`home`, `slideshow`, `network`, `setup`, `updates`, `general`, `monitoring`,
  `services`, `auth`). Each drives the page's actions and asserts the effect on the board
  (panel, shim log, persisted state), not just the toast. `navigation.spec.ts` loads every
  page and fails on any API call that errors. `server-traffic.spec.ts` covers everything
  the server sends over the frames socket: pictures, removals, expiry, clear-display,
  duplicates, garbage, dropped connections and rejected tokens.
- **Virtual panel.** With `display.mock_display` and `display.mock_output_dir` set, the
  display writes every frame to `panel.pbm` (the exact 800×480 1-bit buffer the e-paper
  would get) and `panel.json` (`state`, `source`, `seq`), and counts refreshes like the
  panel. Wait with `waitForPanel(predicate)`; check the frame with `Frame.read()` and
  `blackRatio(...)` (orientation, polarity, dithering) — no reference images in the repo.
  The report attaches the frames as PNG.
- **Shims.** Every command the board shells out to is a script in `board/shims/` that logs
  its arguments to `/state/shim-calls.log`; assert with `expectShimCall(line)`. A new
  `Subprocess::run` call needs a shim, or the e2e board runs the real thing or fails.
  `s6-svc -r` really restarts the service; `shareframe-wifi` keeps its saved networks in
  `/state/wifi-networks`; `shareframe-update-history` appends to the history file.
- **Board state.** `/state` is shared with the runner. `setWifiMode('ap')` switches what
  the Wi-Fi daemon reports; defaults live in `/state/defaults/` and are restored before
  every test. The installed version is `1.2.0` (`/etc/shareframe-release`).
- **Fake server.** It assigns picture ids (`server.sendPicture(fixture)` returns the id)
  and remembers them, so a reset removes them from the frame again. `offerRelease`
  serves a bundle (`corrupt: true` breaks its checksum), `disconnect(code?)` closes or
  drops the frame's socket. A new board → server call needs a route in `server.mjs`.
- **Config.** `board/config.toml` is the complete config of the e2e board (no profile
  overlay); `board/secrets.toml` is a throwaway identity. Both are committed.
- **Isolation.** One board, one worker, alphabetical order. The `cleanBoard` auto fixture
  resets network state and the shim log, removes every pushed picture, restarts the
  slideshow and waits for a default image. A test that changes settings or the password
  restores them itself.
- **UI locks.** The dashboard locks actions for 20 s after load and 3 min after each action.
  `signIn()` and `skipActionLock()` move Playwright's clock past them. The viewport is
  1600×900; at 1280 the open sidebar covers the content.
- **Debugging.** `./test.sh e2e --ui` opens Playwright UI mode on http://localhost:8089;
  `--keep` leaves the stack up (dashboard on http://localhost:8088,
  `docker compose -f tests/e2e/docker-compose.yml logs board`). Containers, images and
  volumes are prefixed `shareframe-board-e2e`; the C++ container `shareframe-board-cpp-test`.
  The HTML report is in `tests/e2e/playwright-report/`.

## Conventions

**Only valuable tests.** Quality over quantity: every test must be able to catch a
realistic bug that no other test catches. Before adding one, name the bug it guards
against; if you can't, don't write it.

- No tests for getters, constants, framework or library behaviour.
- Don't repeat a lower tier: if a contract test pins a mapping, no unit test restates it.
- Prefer one test that walks a real flow over several that each check one line of it.
- Make the inputs discriminating: a test where swapping two fields still passes (both
  services healthy, both values equal) proves nothing.
- A test that is never going to fail, or whose failure nobody would act on, gets deleted.
- A bug fix starts with a test that fails for the reason of the bug.
- Comments say why a test looks unusual, never what the code does. A test that pins a
  known bug rather than the intended behaviour says so — `grep -rn "Known bug"`.
- Test names state the behaviour (`DuplicatePictureIsIgnored`,
  `'a stopped slideshow ignores skip requests'`), not the method under test.
