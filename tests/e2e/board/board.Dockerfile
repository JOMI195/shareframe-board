FROM debian:trixie-slim AS build
RUN apt-get update \
    && apt-get install -y --no-install-recommends \
       build-essential cmake ninja-build git ca-certificates libssl-dev zlib1g-dev \
    && rm -rf /var/lib/apt/lists/*
WORKDIR /src
COPY . .
# Install rules want a profile overlay; the e2e image never installs.
RUN touch config.prod.toml
RUN --mount=type=cache,target=/build \
    cmake -S . -B /build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    && cmake --build /build --target shareframe-websocket shareframe-display shareframe-dashboard \
       shareframe-heartbeat shareframe-update shareframe-migrate \
    && mkdir -p /out && cp /build/shareframe-* /out/

FROM debian:trixie-slim
RUN apt-get update \
    && apt-get install -y --no-install-recommends libssl3t64 zlib1g procps busybox wpasupplicant \
    && rm -rf /var/lib/apt/lists/*
# The image's own scripts, from shareframe-hardware's br2-external (build context "hardware"),
# under ash as on the board; the shims only stand in for hardware and the init system.
RUN ln -sf /bin/busybox /bin/sh
COPY --from=hardware overlay/usr/bin/ /usr/bin/
COPY --from=hardware overlay/usr/lib/shareframe/ /usr/lib/shareframe/
COPY --from=hardware package/shareframe-rauc/files/tryboot-backend /usr/lib/rauc/tryboot-backend
COPY --from=build /out/ /usr/local/bin/
COPY migrations/ /usr/share/shareframe/migrations/
COPY assets/ /usr/share/shareframe/assets/
COPY tests/e2e/board/config.toml /etc/shareframe/config.toml
# A fresh frame identity per image; keys never live in git.
RUN printf 'ed25519_private_key = "%s"\ndashboard_initial_password = "e2e-password"\n' \
    "$(head -c 32 /dev/urandom | base64)" > /etc/shareframe/.env.secrets.toml
COPY tests/e2e/board/shims/ /opt/shims/
COPY tests/e2e/board/entrypoint.sh /usr/local/bin/entrypoint.sh
RUN echo "VERSION=1.2.0" > /etc/shareframe-release
ENV PATH=/opt/shims:$PATH \
    SHAREFRAME_PROFILE=dev \
    CONSOLE=/dev/null \
    CMDLINE=/state/cmdline \
    NO_MOUNT=1 FW_MNT=/state/fw \
    SYSFS=/run/fake-sys \
    LED=/run/fake-sys/class/leds/ACT \
    RESET_MARK=/state/factory-reset
HEALTHCHECK --interval=1s --timeout=2s --retries=60 \
    CMD test -f /panel/panel.json && bash -c '</dev/tcp/127.0.0.1/8080'
ENTRYPOINT ["/usr/local/bin/entrypoint.sh"]
