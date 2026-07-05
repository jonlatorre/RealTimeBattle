#!/usr/bin/env bash
#
# Empaqueta un binario ya compilado junto con las arenas, los robots de
# ejemplo, la documentación y la licencia en un .tar.gz descargable.
# Uso: package.sh [graphics|headless] <version>
#
set -euo pipefail

MODE="${1:-graphics}"
VERSION="${2:-dev}"
ARCH="$(uname -m)"

PKG="RealTimeBattle-${VERSION}-${MODE}-linux-${ARCH}"
DEST="dist/${PKG}"

mkdir -p "$DEST"

# Binario (con sufijo para distinguir el modo headless)
if [ "$MODE" = "headless" ]; then
  cp src/RealTimeBattle "$DEST/RealTimeBattle-headless"
else
  cp src/RealTimeBattle "$DEST/RealTimeBattle"
fi

# Arenas (solo los ficheros de arena, no los Makefile)
mkdir -p "$DEST/Arenas"
cp Arenas/*.arena "$DEST/Arenas/"

# Robots de ejemplo (binarios .robot ya compilados)
mkdir -p "$DEST/Robots"
for d in Robots/*/; do
  name="$(basename "$d")"
  if ls "$d"/*.robot >/dev/null 2>&1; then
    mkdir -p "$DEST/Robots/$name"
    cp "$d"/*.robot "$DEST/Robots/$name/"
  fi
done

# Documentación y licencia
for f in COPYING README ChangeLog BUILD-LINUX-MODERNO.md; do
  [ -f "$f" ] && cp "$f" "$DEST/"
done
if [ -d Documentation ]; then
  mkdir -p "$DEST/Documentation"
  cp Documentation/*.html Documentation/*.txt Documentation/*.gif \
     "$DEST/Documentation/" 2>/dev/null || true
fi

# Tarball
( cd dist && tar czf "${PKG}.tar.gz" "${PKG}" && rm -rf "${PKG}" )

echo "==> creado dist/${PKG}.tar.gz"
ls -la "dist/${PKG}.tar.gz"
