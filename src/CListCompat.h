/*
RealTimeBattle, a robot programming game for Unix
Copyright (C) 1998-2000  Erik Ouchterlony and Ragnar Ouchterlony

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software Foundation,
Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

//
// CListCompat: una capa de compatibilidad que reimplementa el subconjunto
// del API de GtkCList que usa RealTimeBattle sobre GtkTreeView + GtkListStore.
// GtkCList fue eliminado por completo en GTK 3; en lugar de reescribir cada
// ventana a GtkTreeView idiomatico, este shim centraliza la traduccion y deja
// el codigo de las ventanas practicamente intacto.
//
// El "clist" es en realidad un GtkScrolledWindow que contiene un GtkTreeView.
// El GtkListStore tiene, para N columnas visibles:
//   [0]        GdkPixbuf   (icono de la columna 0)
//   [1..N]     gchararray  (texto de las columnas visibles 0..N-1)
//   [N+1]      GdkRGBA     (color de primer plano de la fila)
//   [N+2]      gboolean    (primer plano activo)
//   [N+3]      GdkRGBA     (color de fondo de la fila)
//   [N+4]      gboolean    (fondo activo)
//

#ifndef __CLIST_COMPAT__
#define __CLIST_COMPAT__

#ifndef NO_GRAPHICS

#include <gtk/gtk.h>

// En el codigo original las llamadas son gtk_clist_xxx( GTK_CLIST(w), ... ).
// Aqui GTK_CLIST es simplemente el GtkWidget* (el scrolled window envolvente).
#undef GTK_CLIST
#define GTK_CLIST(x) ((GtkWidget*)(x))

// Firma de la funcion de comparacion para la ordenacion. Recibe el texto de
// la columna de orden de dos filas y devuelve <0, 0, >0.
typedef gint (*GtkClistCompareFunc)( const gchar* text1, const gchar* text2 );

// Callbacks de seleccion (equivalentes a las senales select_row/unselect_row
// y click_column de GtkCList).
typedef void (*GtkClistSelectFunc)( GtkWidget* clist, gint row, gint column,
                                    GdkEventButton* event, gpointer data );
typedef void (*GtkClistClickColumnFunc)( GtkWidget* clist, gint column,
                                         gpointer data );

GtkWidget* gtk_clist_new_with_titles( gint columns, gchar* titles[] );

void gtk_clist_set_selection_mode( GtkWidget* clist, GtkSelectionMode mode );
void gtk_clist_set_column_width( GtkWidget* clist, gint column, gint width );
void gtk_clist_set_column_justification( GtkWidget* clist, gint column,
                                         GtkJustification justification );
void gtk_clist_set_column_title( GtkWidget* clist, gint column,
                                 const gchar* title );
void gtk_clist_set_column_auto_resize( GtkWidget* clist, gint column,
                                       gboolean auto_resize );
void gtk_clist_set_column_resizeable( GtkWidget* clist, gint column,
                                      gboolean resizeable );
void gtk_clist_set_column_max_width( GtkWidget* clist, gint column,
                                     gint max_width );
void gtk_clist_set_column_visibility( GtkWidget* clist, gint column,
                                      gboolean visible );
void gtk_clist_column_titles_passive( GtkWidget* clist );
void gtk_clist_column_title_passive( GtkWidget* clist, gint column );
void gtk_clist_column_title_active( GtkWidget* clist, gint column );
void gtk_clist_columns_autosize( GtkWidget* clist );

// No-ops (propiedades del marco/scroll que ya no aplican).
void gtk_clist_set_shadow_type( GtkWidget* clist, GtkShadowType type );
void gtk_clist_set_border( GtkWidget* clist, GtkShadowType type );
void gtk_clist_set_policy( GtkWidget* clist, GtkPolicyType h, GtkPolicyType v );
void gtk_clist_freeze( GtkWidget* clist );
void gtk_clist_thaw( GtkWidget* clist );

gint gtk_clist_append( GtkWidget* clist, gchar* text[] );
gint gtk_clist_insert( GtkWidget* clist, gint row, gchar* text[] );
void gtk_clist_remove( GtkWidget* clist, gint row );
void gtk_clist_clear( GtkWidget* clist );

void gtk_clist_set_text( GtkWidget* clist, gint row, gint column,
                         const gchar* text );
gint gtk_clist_get_text( GtkWidget* clist, gint row, gint column,
                         gchar** text );
void gtk_clist_set_pixbuf( GtkWidget* clist, gint row, gint column,
                           GdkPixbuf* pixbuf );
void gtk_clist_set_foreground( GtkWidget* clist, gint row,
                               const GdkColor* colour );
void gtk_clist_set_background( GtkWidget* clist, gint row,
                              const GdkColor* colour );

void gtk_clist_select_row( GtkWidget* clist, gint row, gint column );
void gtk_clist_unselect_row( GtkWidget* clist, gint row, gint column );

void gtk_clist_set_compare_func( GtkWidget* clist, GtkClistCompareFunc func );
void gtk_clist_set_sort_column( GtkWidget* clist, gint column );
void gtk_clist_set_sort_type( GtkWidget* clist, GtkSortType type );
void gtk_clist_sort( GtkWidget* clist );

// Registro de callbacks (sustituye a g_signal_connect sobre las senales
// select_row / unselect_row / click_column de GtkCList).
void gtk_clist_set_select_callback( GtkWidget* clist,
                                    GtkClistSelectFunc select_cb,
                                    GtkClistSelectFunc unselect_cb,
                                    gpointer data );
void gtk_clist_set_click_column_callback( GtkWidget* clist,
                                          GtkClistClickColumnFunc cb,
                                          gpointer data );

#endif // NO_GRAPHICS

#endif // __CLIST_COMPAT__
