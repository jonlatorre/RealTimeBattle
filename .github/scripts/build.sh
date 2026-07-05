#!/usr/bin/env bash
#
# Compila RealTimeBattle en un entorno Linux moderno.
# Uso: build.sh [graphics|headless]
#
# Los flags relajan el estándar de C (el configure de 2002 usa main() estilo
# K&R que gcc moderno rechazaría) y activan -fpermissive para el C++ antiguo.
#
set -euo pipefail

MODE="${1:-graphics}"

export CC="gcc"
export CFLAGS="-O2 -std=gnu17 -Wno-implicit-int -Wno-implicit-function-declaration -Wno-int-conversion"
export CXXFLAGS="-O2 -fpermissive -w"

echo "==> configure ($MODE)"
if [ "$MODE" = "headless" ]; then
  ./configure --disable-graphics
else
  ./configure
fi

echo "==> make intl/libintl.a"
make -C intl libintl.a

echo "==> make src (motor del juego)"
make -C src

echo "==> make Robots (robots de ejemplo)"
make -C Robots

echo "==> binario:"
ls -la src/RealTimeBattle
