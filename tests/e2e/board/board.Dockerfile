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
    && apt-get install -y --no-install-recommends libssl3t64 zlib1g procps \
    && rm -rf /var/lib/apt/lists/*
COPY --from=build /out/ /usr/local/bin/
COPY migrations/ /usr/share/shareframe/migrations/
COPY assets/ /usr/share/shareframe/assets/
COPY tests/e2e/board/config.toml /etc/shareframe/config.toml
COPY tests/e2e/board/secrets.toml /etc/shareframe/.env.secrets.toml
COPY tests/e2e/board/shims/ /opt/shims/
COPY tests/e2e/board/entrypoint.sh /usr/local/bin/entrypoint.sh
RUN echo "VERSION=1.2.0" > /etc/shareframe-release
ENV PATH=/opt/shims:$PATH \
    SHAREFRAME_PROFILE=dev
HEALTHCHECK --interval=1s --timeout=2s --retries=60 \
    CMD test -f /panel/panel.json && bash -c '</dev/tcp/127.0.0.1/8080'
ENTRYPOINT ["/usr/local/bin/entrypoint.sh"]
