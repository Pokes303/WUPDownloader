#!/bin/bash
set -e

sudo apt-get update

if ! type aria2c >/dev/null 2>&1; then
  sudo apt-get install -y aria2
fi

wget https://raw.githubusercontent.com/ilikenwf/apt-fast/master/apt-fast -O /usr/local/sbin/apt-fast
chmod +x /usr/local/sbin/apt-fast
wget https://raw.githubusercontent.com/V10lator/NUSspli/master/apt-fast/apt-fast.conf -O /etc/apt-fast.conf
