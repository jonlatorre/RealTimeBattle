# Compilación de RealTimeBattle 1.0.5 en Linux moderno

Este proyecto es de ~2002 y originalmente dependía de **GTK+ 1.x** y de un C++
pre-estándar. Se ha portado para compilar con **g++ 15 / Ubuntu 26.04** en dos
modos:

- **Con interfaz gráfica**, portada de **GTK+ 1.x a GTK 3**.
- **Sin gráficos** (headless), el modo pensado para torneos automatizados.

## Dependencias

```sh
# Para el modo gráfico (GTK 3) se necesita además libgtk-3-dev:
sudo apt-get install -y build-essential autoconf automake pkg-config libgtk-3-dev
```

## Compilar

Los flags relajan el estándar de C (el `configure` de 2002 usa `main()` estilo
K&R que g++ 15 rechazaría) y activan `-fpermissive` para el C++ antiguo:

```sh
export CC="gcc"
export CFLAGS="-O2 -std=gnu17 -Wno-implicit-int -Wno-implicit-function-declaration -Wno-int-conversion"
export CXXFLAGS="-O2 -fpermissive -w"

./configure                     # con interfaz gráfica GTK 3
#   ó
./configure --disable-graphics  # headless

make
```

En modo gráfico, `configure` detecta GTK 3 con `pkg-config gtk+-3.0`.

Resultado:
- `src/RealTimeBattle` — el motor del juego (con o sin GUI según el `configure`).
- `Robots/*/*.robot` — robots de ejemplo (procesos independientes).

## Integración continua y binarios (GitHub Actions)

El repositorio incluye dos flujos de trabajo en `.github/workflows/`, que
reutilizan los scripts de `.github/scripts/` (`build.sh`, `smoke-test.sh`,
`package.sh`):

- **`ci.yml`** — en cada *push* y *pull request* a `master`: compila los dos
  modos (gráfico y headless), ejecuta una prueba de humo (un torneo corto que
  debe generar estadísticas) y publica los `.tar.gz` como **artefactos** de la
  ejecución.
- **`release.yml`** — al crear una etiqueta `v*` (o manualmente desde la pestaña
  Actions): compila ambos binarios, genera además un tarball del **código
  fuente** y lo sube todo como *assets* de una **GitHub Release** (equivalente a
  las descargas por versión que el proyecto tenía en SourceForge).

Para publicar una versión basta con:

```sh
git tag v1.0.5-modern
git push origin v1.0.5-modern
```

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

## Portado de la interfaz gráfica a GTK 3

La GUI original (GTK+ 1.x) se ha portado a **GTK 3.24**. Los cambios principales:

1. **Detección de GTK**: `configure.in` usaba `AM_PATH_GTK` (busca el `gtk-config`
   de GTK 1.x, inexistente hoy). Se sustituyó por `PKG_CHECK_MODULES(GTK,
   gtk+-3.0)` (y el bloque equivalente en el `configure` generado).
2. **Dibujo del arena** (`ArenaWindow.cc`): GTK 3 elimina el dibujo inmediato
   (`GdkGC` + `gdk_draw_*`). Se reescribió con **Cairo** sobre una *superficie de
   respaldo* (`cairo_surface_t`): las primitivas dibujan en la superficie y el
   callback `draw` la vuelca en pantalla. Los colores `GdkColor` se convierten a
   componentes Cairo (0..1).
3. **Listas `GtkCList` → `GtkTreeView`**: `GtkCList` fue eliminado por completo.
   En vez de reescribir cada ventana, se creó un **shim de compatibilidad**
   (`CListCompat.h/.cc`) que reimplementa el subconjunto usado del API
   `gtk_clist_*` sobre `GtkTreeView` + `GtkListStore`. Las ventanas de score,
   estadísticas, torneo y mensajes quedan casi intactas.
4. **Iconos de color** (`Structs.cc`, `pixmap_t`): `GdkPixmap`/`GdkBitmap` →
   `GdkPixbuf` (mostrado con `GtkCellRendererPixbuf`).
5. **Señales**: `gtk_signal_connect`/`GTK_OBJECT`/`GtkSignalFunc` (todo el API
   `gtk_signal_*`, eliminado) → `g_signal_connect`/`G_OBJECT`/`G_CALLBACK`.
6. **Widgets eliminados**: `GtkOptionMenu` → `GtkMenuBar`+`GtkCheckMenuItem`;
   `GtkFileSelection` → `GtkFileChooserDialog`; `gtk_vbox/hbox_new` →
   `gtk_box_new(GTK_ORIENTATION_*, …)`; `gtk_pixmap_new` → `GtkImage`;
   `GtkStyle`/`gtk_rc_get_style` (color de listas) → eliminado.
7. **Otros renames**: `gtk_timeout_add`→`g_timeout_add`,
   `gtk_widget_set_usize`→`gtk_widget_set_size_request`,
   `gtk_widget_set_uposition`→`gtk_window_move`,
   `gtk_container_border_width`→`gtk_container_set_border_width`,
   `gtk_set_locale`→`setlocale`, `GdkColormap`/`gdk_color_alloc` eliminados,
   accesos a structs opacos (`widget->window`, `adj->value`, …) → *getters*.
8. **Cabeceras**: los `#include <gdk/gdktypes.h>` / `<gtk/gtkwidget.h>` parciales
   (prohibidos en GTK 3) → `<gdk/gdk.h>` / `<gtk/gtk.h>`.

Verificado con `Xvfb`: la versión gráfica arranca y ejecuta un torneo completo
sin errores ni mensajes `CRITICAL`, generando el mismo fichero de estadísticas
que la versión headless.
