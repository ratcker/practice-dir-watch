#!/usr/bin/env bash

set -Eeuo pipefail

readonly BINARY_NAME="dir-watch-bin"
readonly COMMAND_NAME="dir-watch"
readonly SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"

PREFIX="${PREFIX:-/usr/local}"

usage() {
    cat <<EOF
Usage: ./install [--prefix <path>]

Options:
  --prefix / -p <path>   Installation prefix (default /usr/local)
  --help / -h            Show help massage

Example:
  ./install.sh
  ./install.sh --prefix "\$HOME/.local"
EOF
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        -p|--prefix)
            if [[ $# -lt 2 ]]; then
                echo "Error: --prefix requires a path" >&2
                exit 2
            fi
            PREFIX="$2"
            shift 2
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            echo "Error: unknown option $1" >&2
            usage >&2
            exit 2
            ;;
    esac
done

readonly SOURCE_FILE="$SCRIPT_DIR/$BINARY_NAME"
readonly TARGET_DIR="$PREFIX/bin"
readonly TARGET_FILE="$TARGET_DIR/$BINARY_NAME"
readonly COMMAND_LINK="$TARGET_DIR/$COMMAND_NAME"

if [[ ! -f "$SOURCE_FILE" ]]; then
    echo "Error: $SOURCE_FILE was not found" >&2
    exit 1
fi

run_install() {
    install -d "$TARGET_DIR"
    install -m 0755 "$SOURCE_FILE" "$TARGET_FILE"
    ln -sfn "$BINARY_NAME" "$COMMAND_LINK"
}

can_install_without_sudo() {
    local existing_path="$TARGET_DIR"
    while [[ ! -e "$existing_path" ]] && [[ "$existing_path" != "/" ]]; do
        existing_path="$(dirname "$existing_path")"
    done
    [[ -w "$existing_path" ]]
}

if [[ $EUID -eq 0 ]] || can_install_without_sudo; then
    run_install
else
    if ! command -v sudo >/dev/null 2>&1; then
        echo "Error: cannot write to $TARGET_DIR and sudo is not available" >&2
        exit 1
    fi

    sudo install -d "$TARGET_DIR"
    sudo install -m 0755 "$SOURCE_FILE" "$TARGET_FILE"
    sudo ln -sfn "$BINARY_NAME" "$COMMAND_LINK"
fi

echo "Intalled $TARGET_FILE"
echo "Command_available as: $COMMAND_LINK"
echo "Run: dir-watch --help"