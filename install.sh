#!/bin/bash

BINARY_NAME="dir-watch"
TARGET_DIR="/usr/local/bin"
echo " dir-watch installer "
read -p "Are you agree with install of $BINARY_NAME in $TARGET_DIR? [y/n]: " response
response=$(echo "$response" | tr '[:upper:]' '[:lower:]')
if [[ "$response" != "y" && "$response" != "yes" ]]; then
    echo "Cancelled"
    exit 0
fi
if [ "$EUID" -ne 0 ]; then
    echo "Error: you need to use sudo for install"
    exit 1
fi
if [ ! -f "$BINARY_NAME" ]; then
    echo "Error: file of $BINARY_NAME does not found in this directory"
    exit 1
fi
echo "installing.."
chmod +x "$BINARY_NAME"
cp "$BINARY_NAME" "$TARGET_DIR/"
if [ $? -eq 0 ]; then
    echo "installing successfull completed"
    echo "use dir-watch --help to get info about dir-watch"
else
    echo "unknown error"
    exit 1
fi