#!/bin/sh
# Fetch build dependencies into this checkout, never into system directories.
# Prerequisites: POSIX shell, curl, tar, jq, sha512sum, Python 3 (Sphinx only).
# No project build/translation/demo logic is implemented in Python.
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
mode=${1:-all}
mkdir -p "$root/.toolchain"
if [ "$mode" = dotnet ] || [ "$mode" = all ]; then
    if ! command -v dotnet >/dev/null 2>&1 && [ ! -x "$root/.toolchain/dotnet/dotnet" ]; then
        case "$(uname -s):$(uname -m)" in Linux:x86_64) ;; *) echo 'Install a .NET 8 SDK for your platform and put dotnet on PATH.' >&2; exit 1;; esac
        # Use Microsoft's release manifest to verify the SDK archive's SHA-512.
        version=8.0.413
        curl --fail --location --retry 2 'https://builds.dotnet.microsoft.com/dotnet/release-metadata/8.0/releases.json' -o "$root/.toolchain/dotnet-releases.json"
        archive_url=$(jq -r --arg ver "$version" '[.releases[].sdks[]? | select(.version==$ver) | .files[] | select(.rid=="linux-x64" and (.name|endswith(".tar.gz")))][0].url' "$root/.toolchain/dotnet-releases.json")
        archive_hash=$(jq -r --arg ver "$version" '[.releases[].sdks[]? | select(.version==$ver) | .files[] | select(.rid=="linux-x64" and (.name|endswith(".tar.gz")))][0].hash' "$root/.toolchain/dotnet-releases.json")
        [ "$archive_url" != null ] && [ "$archive_hash" != null ]
        curl --fail --location --retry 2 "$archive_url" -o "$root/.toolchain/dotnet-sdk.tar.gz"
        printf '%s  %s\n' "$archive_hash" "$root/.toolchain/dotnet-sdk.tar.gz" | sha512sum --check -
        mkdir -p "$root/.toolchain/dotnet"
        tar -xzf "$root/.toolchain/dotnet-sdk.tar.gz" -C "$root/.toolchain/dotnet"
    fi
fi
if [ "$mode" = docs ] || [ "$mode" = all ]; then
    python3 -m venv "$root/.toolchain/docs"
    "$root/.toolchain/docs/bin/python" -m pip install -r "$root/docs/requirements.txt"
fi
if [ "$mode" = riscv ]; then
    # Prefer system cross tools; install a pinned Zig compiler only if needed.
    if [ ! -x "$root/.toolchain/zig/zig" ] && [ ! -x "$root/.toolchain/ziglang/zig" ]; then
        curl --fail --location --retry 2 https://ziglang.org/download/0.14.1/zig-x86_64-linux-0.14.1.tar.xz -o "$root/.toolchain/zig.tar.xz"
        mkdir -p "$root/.toolchain/zig"
        tar -xJf "$root/.toolchain/zig.tar.xz" -C "$root/.toolchain/zig" --strip-components=1
    fi
    # The Debian qemu-user package is extracted locally; no root installation.
    if ! command -v qemu-riscv64 >/dev/null 2>&1 && [ ! -x "$root/.toolchain/qemu/usr/bin/qemu-riscv64" ]; then
        curl --fail --location --retry 2 'https://deb.debian.org/debian/pool/main/q/qemu/qemu-user_10.0.13+ds-0+deb13u1_amd64.deb' -o "$root/.toolchain/qemu-user.deb"
        dpkg-deb -x "$root/.toolchain/qemu-user.deb" "$root/.toolchain/qemu"
    fi
fi
