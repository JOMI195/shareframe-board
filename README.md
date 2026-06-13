<img width="1280" height="450" alt="shareframe-board-title" src="PLACEHOLDER_TITLE_IMAGE_URL" />

# Shareframe (Application)

shareframe-board is the C++ application firmware that runs on the Shareframe e-paper picture frames (Raspberry Pi Zero
W). It is a set of small, s6-supervised processes that maintain a persistent WebSocket to the Shareframe web server,
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

<img width="1460" height="1020" alt="shareframe board application architecture" src="PLACEHOLDER_ARCH_IMAGE_URL" />

*Figure 1: Outline of the on-device application architecture of shareframe-board*

**Processes*:

- **shareframe-websocket**: Holds the persistent WebSocket to the server. Ingests and persists shared photos, then
  publishes image events to the display.
- **shareframe-display**: Drives the Waveshare e-paper panel over SPI and runs the slideshow. Subscribes to image events
  and serves display commands over IPC.
- **shareframe-dashboard**: Local web server serving the React dashboard and a REST API. Handles Wi-Fi and system
  actions and proxies display/update commands.
- **shareframe-update**: Polls the server for new releases and installs OTA bundles via the OS update backend.
- **shareframe-heartbeat**: Periodically reports the frame's liveness and status to the server and health-probes the
  other services.
