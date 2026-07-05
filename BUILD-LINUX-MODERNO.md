# Compilación de RealTimeBattle 1.0.5 en Linux moderno

Este proyecto es de ~2002 y originalmente dependía de **GTK+ 1.x** y de un C++
pre-estándar. Se ha portado para compilar con **g++ 15 / Ubuntu 26.04** en modo
**sin gráficos** (headless), que es el modo pensado para torneos automatizados
entre robots. La interfaz gráfica GTK+ 1.x no se ha portado (esa librería ya no
existe en las distros modernas).

## Dependencias

```sh
sudo apt-get install -y build-essential autoconf automake pkg-config
```

## Compilar

Los flags relajan el estándar de C (el `configure` de 2002 usa `main()` estilo
K&R que g++ 15 rechazaría) y activan `-fpermissive` para el C++ antiguo:

```sh
export CC="gcc"
export CFLAGS="-O2 -std=gnu17 -Wno-implicit-int -Wno-implicit-function-declaration -Wno-int-conversion"
export CXXFLAGS="-O2 -fpermissive -w"

./configure --disable-graphics
make
```

Resultado:
- `src/RealTimeBattle` — el motor del juego.
- `Robots/*/*.robot` — robots de ejemplo (procesos independientes).

## Probar (torneo headless)

```sh
RTB=$(pwd)
cat > /tmp/tt.txt <<EOF
Robots: $RTB/Robots/seek_and_destroy/seek_and_destroy.robot $RTB/Robots/rotate_and_fire/rotate_and_fire_signal.robot
Arenas: $RTB/Arenas/Circle.arena
Games/Sequence: 1
Robots/Sequence: 2
Sequences: 1
EOF
printf 'Timeout [s]: 8\n' > /tmp/opts.rtb

./src/RealTimeBattle -g -c -o /tmp/opts.rtb -t /tmp/tt.txt -s /tmp/stats.txt
cat /tmp/stats.txt
```

## Cambios de portado realizados

1. **Cabeceras C++ pre-estándar** → estándar en todos los `.cc`/`.h`:
   `<iostream.h>`→`<iostream>`, `<fstream.h>`→`<fstream>`,
   `<iomanip.h>`→`<iomanip>`, `<strstream.h>`→`<sstream>`. Se añadió
   `using namespace std;`.
2. **`String.cc`**: `strstream` reescrito con `ostringstream`.
3. **`List.h` / `Vector2D.h` / `String.h`**: declaraciones cualificadas dentro
   de la clase corregidas; añadidas declaraciones a nivel de namespace de las
   funciones `friend` que ya no se encuentran por ADL en C++ moderno
   (p.ej. `angle2vec`, `hex2str`, `str2int`).
4. **Apertura de streams**: eliminado el tercer argumento obsoleto de `open()`;
   `.attach(fd)` → `.open("/dev/std*", ...)`.
5. **`Robot.cc`**: construcción de streams desde un descriptor de pipe
   (`new ofstream(fd)`, ya no permitido) reescrita con
   `__gnu_cxx::stdio_filebuf`.
6. **`Robot.cc::get_messages`**: el drenaje no bloqueante de mensajes dependía
   de que `eof()` se activara con `EAGAIN`. Los iostreams modernos ponen
   `failbit` (no `eofbit`), lo que causaba un bucle infinito. El bucle ahora
   termina cuando falla la extracción y limpia el estado en cada tick.
7. **`Various.cc`**: parser del fichero de torneo con el mismo problema de
   `EAGAIN`/`eof`; se limpia el buffer antes de cada lectura.
8. **Robots de ejemplo**: mismas correcciones de cabeceras + `<cstring>`.
