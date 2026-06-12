# shareframe-board

C++ firmware for the Shareframe-Board (Raspberry Pi Zero W): a networked e-paper
photo frame with a local web dashboard. Built into the SD image by the
[shareframe-hardware](../shareframe-hardware) Buildroot project — see that repo
for building, flashing, and OTA releases.

## Services

Several small executables share one SQLite database (on `/data`) and talk to each
other over nng (request/reply + publish/subscribe).

| Binary | Role |
|--------|------|
| `shareframe-display` | Drives the e-paper panel and runs the slideshow; sole owner of the display hardware. |
| `shareframe-websocket` | Talks to the server over WebSocket; ingests images and forwards events to the display. |
| `shareframe-dashboard` | Local HTTP API for the web dashboard — status, slideshow control, Wi-Fi setup, logs, updates. |
| `shareframe-heartbeat` | Periodic status report to the server. |
| `shareframe-update` | A/B over-the-air updates (RAUC + tryboot); the OS owns slot state. |
| `shareframe-migrate` | One-shot database migration runner; runs before any service starts. |

Shared code (config, database, IPC, auth, networking, repositories) lives in the
`shareframe-common` static library.

## Layout

| Path | Contents |
|------|----------|
| `apps/` | One entry point per service binary. |
| `src/` , `include/` | Implementation + headers, grouped by area (display, dashboard, net, ...). |
| `dashboard-frontend/` | React/Vite single-page dashboard, served by nginx. |
| `migrations/` | SQL schema migrations. |
| `external/` | Vendored third-party code (image + e-paper driver). |
| `tests/` | Unit tests. |
| `config*.toml` | Layered config: `config.toml` base + a per-profile overlay (dev/prod) + secrets. |

## Build

The firmware build (in shareframe-hardware) compiles this via Buildroot's
cmake-package. To build standalone for development:

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

`-DENABLE_EPD_HARDWARE=ON` links the real e-paper driver; the default builds a
mock display for host development.
