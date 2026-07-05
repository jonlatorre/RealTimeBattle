#!/usr/bin/env bash
#
# Prueba de humo: ejecuta un torneo corto en modo competición sin gráficos
# (-g) y comprueba que se genera el fichero de estadísticas. Funciona tanto
# con el binario gráfico como con el headless, sin necesidad de un servidor X.
#
set -euo pipefail

RTB="$(pwd)"
TT="$(mktemp)"
OPTS="$(mktemp)"
STATS="$(mktemp)"

cat > "$TT" <<EOF
Robots: $RTB/Robots/seek_and_destroy/seek_and_destroy.robot $RTB/Robots/rotate_and_fire/rotate_and_fire_signal.robot
Arenas: $RTB/Arenas/Circle.arena
Games/Sequence: 1
Robots/Sequence: 2
Sequences: 1
EOF

printf 'Timeout [s]: 5\n' > "$OPTS"

# El binario gráfico llama a gtk_init() aunque se use -g (sin ventanas), por lo
# que necesita un servidor X. Con xvfb-run damos uno virtual; el binario
# headless simplemente lo ignora.
RUN=()
if command -v xvfb-run >/dev/null 2>&1; then
  RUN=( xvfb-run -a )
fi

echo "==> ejecutando torneo de prueba (timeout de guarda 60s)"
timeout 60 "${RUN[@]}" ./src/RealTimeBattle -g -c -o "$OPTS" -t "$TT" -s "$STATS"

echo "---- estadísticas ----"
cat "$STATS"
echo "----------------------"

test -s "$STATS"
grep -q 'Points' "$STATS"
echo "Smoke test OK"
