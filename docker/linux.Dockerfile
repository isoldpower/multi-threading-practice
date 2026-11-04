FROM ubuntu:24.04

# Mirrors the GitHub ubuntu-24.04 runner
RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
        ca-certificates cmake make g++ g++-14 clang clang-tidy git \
    && rm -rf /var/lib/apt/lists/*
