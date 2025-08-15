FROM ghcr.io/eclipse-score/devcontainer:latest

RUN apt-get update && apt-get install -y \
    libclang-dev \
    cargo-1.82 rustc-1.82 \
    && rm -rf /var/lib/apt/lists/*

RUN update-alternatives --install /usr/bin/cargo cargo /usr/bin/cargo-1.82 100 \
    --slave /usr/bin/rustc rustc /usr/bin/rustc-1.82 \
    --slave /usr/bin/rustdoc rustdoc /usr/bin/rustdoc-1.82
