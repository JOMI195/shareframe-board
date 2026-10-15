FROM debian:trixie-slim

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
       build-essential cmake ninja-build git ca-certificates libssl-dev zlib1g-dev gcovr \
    && rm -rf /var/lib/apt/lists/*

ENV CMAKE_GENERATOR=Ninja \
    HOME=/tmp
WORKDIR /src
