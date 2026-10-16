FROM ubuntu:24.04

RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
        ca-certificates cmake make g++ clang clang-tidy git \
    && rm -rf /var/lib/apt/lists/*
