<img width="1280" height="324" alt="shareframe-title-hero" src="https://github.com/user-attachments/assets/78691be7-4e8b-4288-b248-9152f9c5b997" />

# Shareframe (Application)


shareframe-board is the C++ application firmware that runs on the Shareframe e-paper picture frames (Raspberry Pi Zero
W). It is a set of small processes that maintain a persistent WebSocket to the Shareframe web server,
receive the photos shared to the frame, and render them on the e-paper panel as a slideshow. It also serves a local web
dashboard for setup and management, and handles Wi-Fi configuration and over-the-air updates.

## Related repos

- [shareframe](https://github.com/JOMI195/shareframe) — web application platform: server backend and React web frontend
- [shareframe-hardware](https://github.com/JOMI195/shareframe-hardware) — custom embedded Buildroot Linux OS that builds
  the SD card image for the board

## Highlights

- Multi-process firmware. Independent services
- Photo delivery over a persistent WebSocket, shown as an e-paper slideshow
- Local React web dashboard (served on the board) for setup, Wi-Fi, and frame management
- A/B over-the-air updates via the OS RAUC + tryboot backend
- Shared SQLite database and image store on the writable `/data` partition

## Architecture

<img width="1682" height="1305" alt="shareframe-board-architecture drawio" src="https://github.com/user-attachments/assets/2cfdc757-9e41-43de-af3d-eccb25201e79" />

*Figure 1: Outline of the on-device application architecture of shareframe-board*

**Processes**:

- **shareframe-websocket**: Holds the persistent WebSocket to the server. Ingests and persists shared photos, then
  publishes image events to the display.
- **shareframe-display**: Drives the Waveshare e-paper panel over SPI and runs the slideshow. Subscribes to image events
  and serves display commands over IPC.
- **shareframe-dashboard**: Local web server serving the React dashboard and a REST API. Handles Wi-Fi and system
  actions and proxies display/update commands.
- **shareframe-update**: Polls the server for new releases and installs OTA bundles via the OS update backend.
- **shareframe-heartbeat**: Periodically reports the frame's liveness and status to the server and health-probes the
  other services.

**State machines**: every machine keeps its states and rules in one header of pure functions
(`canMove`, `name`, plus the decisions), tested without a thread or a panel. The owning class collects
the inputs, asks the rules and carries out the answer; `fsm::Machine` (`include/fsm/Machine.hpp`)
holds the state and refuses a move the rules forbid.

| Machine | Rules | Owner |
|---|---|---|
| Which screen the panel shows | `include/display/ScreenSelection.hpp` | `DisplayTaskManager` |
| What the slideshow loop is busy with | `include/display/Slideshow.hpp` | `DisplayImageLoop` |
| Panel power, content and health | `include/display/PanelState.hpp` | `DisplayManager` |

A new machine follows the same shape; a new screen is one value in `Screen`, one line in `select`
and one in `DisplayTaskManager::_task`.

## Local board dashboard

<img width="1060" height="870" alt="local embedded board dashboard" src="https://github.com/user-attachments/assets/7c65148e-716f-4842-9afa-c260b3ab664c" />

*Figure 2: Quick overview of the local picture frame management dashboard. It provides controls and give information about the health and state of the picture frame*

