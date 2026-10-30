#!/usr/bin/env bash
# Install the dense CLI from GitHub Releases.
#
#   curl -fsSL https://raw.githubusercontent.com/c-aly/dense/main/scripts/install.sh | bash
#
# Env overrides:
#   DENSE_VERSION   tag without leading v, or "latest" (default)
#   DENSE_REPO      owner/name (default c-aly/dense)
#   DENSE_INSTALL_DIR  where to put the binary (default: /usr/local/bin or ~/.local/bin)
set -euo pipefail

REPO="${DENSE_REPO:-c-aly/dense}"
VERSION="${DENSE_VERSION:-latest}"

say()  { printf 'dense: %s\n' "$*"; }
fail() { printf 'dense: %s\n' "$*" >&2; exit 1; }

need() { command -v "$1" >/dev/null 2>&1 || fail "need '$1' on PATH"; }
need curl
need tar
need uname

os="$(uname -s | tr '[:upper:]' '[:lower:]')"
arch="$(uname -m)"
case "$os" in
  linux)  os=linux ;;
  darwin) os=macos ;;
  *) fail "unsupported OS: $(uname -s) (use install.ps1 on Windows)" ;;
esac
case "$arch" in
  x86_64|amd64) arch=amd64 ;;
  arm64|aarch64) arch=arm64 ;;
  *) fail "unsupported arch: $arch" ;;
esac

asset="dense-${os}-${arch}.tar.gz"

if [ "$VERSION" = "latest" ]; then
  url="https://github.com/${REPO}/releases/latest/download/${asset}"
else
  ver="${VERSION#v}"
  url="https://github.com/${REPO}/releases/download/v${ver}/${asset}"
fi

# Prefer a directory already on PATH and writable.
pick_dir() {
  if [ -n "${DENSE_INSTALL_DIR:-}" ]; then
    printf '%s' "$DENSE_INSTALL_DIR"
    return
  fi
  for d in /usr/local/bin "$HOME/.local/bin"; do
    if [ -d "$d" ] && [ -w "$d" ]; then
      printf '%s' "$d"
      return
    fi
  done
  # Create ~/.local/bin if needed (common user-local PATH entry).
  mkdir -p "$HOME/.local/bin"
  printf '%s' "$HOME/.local/bin"
}

dest_dir="$(pick_dir)"
tmpdir="$(mktemp -d)"
trap 'rm -rf "$tmpdir"' EXIT

say "downloading ${asset} (${VERSION})…"
curl -fsSL "$url" -o "$tmpdir/$asset" \
  || fail "download failed — is there a release with ${asset}? (${url})"

tar -xzf "$tmpdir/$asset" -C "$tmpdir"
bin="$(find "$tmpdir" -type f -name dense | head -n1)"
[ -n "$bin" ] || fail "archive had no 'dense' binary"
chmod +x "$bin"

mkdir -p "$dest_dir"
# /usr/local/bin may need sudo if we somehow got an unwritable pick — shouldn't happen.
if [ -w "$dest_dir" ]; then
  cp "$bin" "$dest_dir/dense"
else
  need sudo
  sudo cp "$bin" "$dest_dir/dense"
fi

say "installed → ${dest_dir}/dense"

case ":$PATH:" in
  *":${dest_dir}:"*) ;;
  *)
    say "note: ${dest_dir} is not on your PATH."
    say "add this to your shell config, then open a new terminal:"
    say "  export PATH=\"${dest_dir}:\$PATH\""
    ;;
esac

say "try: dense --help"
