#! /bin/bash

platform="$1"
arch="$2"

if [ -n "$platform" ] && [ -z "$arch" ]; then
	echo "not specify arch"
	exit 1
fi

if [ -z "$platform" ]; then
	unameOut="$(uname -s)"
	case "${unameOut}" in
	Linux) platform=linux ;;
	Darwin) platform=macos ;;
	CYGWIN* | MINGW* | MSYS*) platform=windows ;;
	*)  echo "unknown platform: ${unameOut}"; exit 1 ;;
	esac

	arch="$(uname -m)"
	echo "platform: ${platform} arch: ${arch}"
fi

url="https://github.com/frida/frida/releases/download/16.1.3/frida-gum-devkit-16.1.3-${platform}-${arch}.tar.xz"
wget "$url" -O frida.tar.xz
rm -rf frida
mkdir frida
tar xf frida.tar.xz -C frida
rm frida.tar.xz