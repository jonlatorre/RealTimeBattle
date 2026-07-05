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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifndef NO_GRAPHICS

#include <string.h>
#include <stdlib.h>
#include "CListCompat.h"

// Indices de columna del store dependientes del numero de columnas visibles n.
#define STORE_PIXBUF   0
#define STORE_TEXT(c)  ((c) + 1)
#define STORE_FG(n)    ((n) + 1)
#define STORE_FG_SET(n) ((n) + 2)
#define STORE_BG(n)    ((n) + 3)
#define STORE_BG_SET(n) ((n) + 4)
#define STORE_NCOLS(n) ((n) + 5)

// ---- Acceso a los objetos internos guardados en el scrolled window --------

static GtkTreeView*
clist_treeview( GtkWidget* clist )
{
  return GTK_TREE_VIEW( g_object_get_data( G_OBJECT( clist ), "rtb-treeview" ) );
}

static GtkListStore*
clist_store( GtkWidget* clist )
{
  return GTK_LIST_STORE( g_object_get_data( G_OBJECT( clist ), "rtb-store" ) );
}

static gint
clist_ncols( GtkWidget* clist )
{
  return GPOINTER_TO_INT( g_object_get_data( G_OBJECT( clist ), "rtb-ncols" ) );
}

static gboolean
clist_nth_iter( GtkWidget* clist, gint row, GtkTreeIter* iter )
{
  GtkTreeModel* model = GTK_TREE_MODEL( clist_store( clist ) );
  return gtk_tree_model_iter_nth_child( model, iter, NULL, row );
}

static void
gdkcolor_to_rgba( const GdkColor* col, GdkRGBA* rgba )
{
  rgba->red   = col->red   / 65535.0;
  rgba->green = col->green / 65535.0;
  rgba->blue  = col->blue  / 65535.0;
  rgba->alpha = 1.0;
}

// Recupera el CellRendererText de una columna visible (para justificacion).
static GtkCellRenderer*
clist_text_renderer( GtkWidget* clist, gint column )
{
  GtkTreeViewColumn* col =
    gtk_tree_view_get_column( clist_treeview( clist ), column );
  if( col == NULL ) return NULL;

  GtkCellRenderer* found = NULL;
  GList* cells = gtk_cell_layout_get_cells( GTK_CELL_LAYOUT( col ) );
  for( GList* l = cells; l != NULL; l = l->next )
    if( GTK_IS_CELL_RENDERER_TEXT( l->data ) )
      found = GTK_CELL_RENDERER( l->data );
  g_list_free( cells );
  return found;
}

// ---- Senal de seleccion --------------------------------------------------

static void
clist_selection_changed( GtkTreeSelection* selection, gpointer user_data )
{
  GtkWidget* clist = GTK_WIDGET( user_data );
  GtkClistSelectFunc cb =
    (GtkClistSelectFunc) g_object_get_data( G_OBJECT( clist ), "rtb-select-cb" );
  if( cb == NULL ) return;

  gpointer data = g_object_get_data( G_OBJECT( clist ), "rtb-select-data" );

  GtkTreeModel* model;
  GtkTreeIter iter;
  if( gtk_tree_selection_get_selected( selection, &model, &iter ) )
    {
      GtkTreePath* path = gtk_tree_model_get_path( model, &iter );
      gint row = gtk_tree_path_get_indices( path )[0];
      gtk_tree_path_free( path );
      // Se fabrica un evento no nulo: el codigo original solo comprueba que
      // event != NULL antes de actuar.
      GdkEventButton ev;
      memset( &ev, 0, sizeof( ev ) );
      ev.type = GDK_BUTTON_PRESS;
      cb( clist, row, 0, &ev, data );
    }
}

// ---- Creacion ------------------------------------------------------------

GtkWidget*
gtk_clist_new_with_titles( gint columns, gchar* titles[] )
{
  gint n = columns;

  // Tipos de las columnas del store.
  GType* types = g_new( GType, STORE_NCOLS( n ) );
  types[STORE_PIXBUF] = GDK_TYPE_PIXBUF;
  for( gint c = 0; c < n; c++ )
    types[STORE_TEXT( c )] = G_TYPE_STRING;
  types[STORE_FG( n )]     = GDK_TYPE_RGBA;
  types[STORE_FG_SET( n )] = G_TYPE_BOOLEAN;
  types[STORE_BG( n )]     = GDK_TYPE_RGBA;
  types[STORE_BG_SET( n )] = G_TYPE_BOOLEAN;

  GtkListStore* store =
    gtk_list_store_newv( STORE_NCOLS( n ), types );
  g_free( types );

  GtkWidget* treeview =
    gtk_tree_view_new_with_model( GTK_TREE_MODEL( store ) );
  gtk_tree_view_set_headers_visible( GTK_TREE_VIEW( treeview ), TRUE );

  for( gint c = 0; c < n; c++ )
    {
      GtkTreeViewColumn* col = gtk_tree_view_column_new();
      gtk_tree_view_column_set_title( col,
                                      titles[c] != NULL ? titles[c] : "" );

      if( c == 0 )
        {
          GtkCellRenderer* pix = gtk_cell_renderer_pixbuf_new();
          gtk_tree_view_column_pack_start( col, pix, FALSE );
          gtk_tree_view_column_add_attribute( col, pix, "pixbuf",
                                              STORE_PIXBUF );
        }

      GtkCellRenderer* txt = gtk_cell_renderer_text_new();
      gtk_tree_view_column_pack_start( col, txt, TRUE );
      gtk_tree_view_column_add_attribute( col, txt, "text", STORE_TEXT( c ) );
      gtk_tree_view_column_add_attribute( col, txt, "foreground-rgba",
                                          STORE_FG( n ) );
      gtk_tree_view_column_add_attribute( col, txt, "foreground-set",
                                          STORE_FG_SET( n ) );
      gtk_tree_view_column_add_attribute( col, txt, "cell-background-rgba",
                                          STORE_BG( n ) );
      gtk_tree_view_column_add_attribute( col, txt, "cell-background-set",
                                          STORE_BG_SET( n ) );

      gtk_tree_view_column_set_sizing( col, GTK_TREE_VIEW_COLUMN_FIXED );
      gtk_tree_view_column_set_resizable( col, TRUE );
      gtk_tree_view_append_column( GTK_TREE_VIEW( treeview ), col );
    }

  GtkWidget* scrolled = gtk_scrolled_window_new( NULL, NULL );
  gtk_scrolled_window_set_policy( GTK_SCROLLED_WINDOW( scrolled ),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC );
  gtk_container_add( GTK_CONTAINER( scrolled ), treeview );
  gtk_widget_show( treeview );

  g_object_unref( store );

  g_object_set_data( G_OBJECT( scrolled ), "rtb-treeview", treeview );
  g_object_set_data( G_OBJECT( scrolled ), "rtb-store", store );
  g_object_set_data( G_OBJECT( scrolled ), "rtb-ncols", GINT_TO_POINTER( n ) );
  g_object_set_data( G_OBJECT( scrolled ), "rtb-sortcol", GINT_TO_POINTER( 0 ) );
  g_object_set_data( G_OBJECT( scrolled ), "rtb-sorttype",
                     GINT_TO_POINTER( GTK_SORT_ASCENDING ) );

  return scrolled;
}

// ---- Columnas ------------------------------------------------------------

void
gtk_clist_set_selection_mode( GtkWidget* clist, GtkSelectionMode mode )
{
  GtkTreeSelection* sel =
    gtk_tree_view_get_selection( clist_treeview( clist ) );
  gtk_tree_selection_set_mode( sel, mode );
}

void
gtk_clist_set_column_width( GtkWidget* clist, gint column, gint width )
{
  GtkTreeViewColumn* col =
    gtk_tree_view_get_column( clist_treeview( clist ), column );
  if( col != NULL )
    {
      gtk_tree_view_column_set_sizing( col, GTK_TREE_VIEW_COLUMN_FIXED );
      gtk_tree_view_column_set_fixed_width( col, width > 0 ? width : 1 );
    }
}

void
gtk_clist_set_column_justification( GtkWidget* clist, gint column,
                                    GtkJustification justification )
{
  gfloat xalign = 0.0;
  if( justification == GTK_JUSTIFY_CENTER ) xalign = 0.5;
  else if( justification == GTK_JUSTIFY_RIGHT ) xalign = 1.0;

  GtkCellRenderer* txt = clist_text_renderer( clist, column );
  if( txt != NULL )
    g_object_set( txt, "xalign", xalign, NULL );

  GtkTreeViewColumn* col =
    gtk_tree_view_get_column( clist_treeview( clist ), column );
  if( col != NULL )
    gtk_tree_view_column_set_alignment( col, xalign );
}

void
gtk_clist_set_column_title( GtkWidget* clist, gint column, const gchar* title )
{
  GtkTreeViewColumn* col =
    gtk_tree_view_get_column( clist_treeview( clist ), column );
  if( col != NULL )
    gtk_tree_view_column_set_title( col, title != NULL ? title : "" );
}

void
gtk_clist_set_column_auto_resize( GtkWidget* clist, gint column,
                                  gboolean auto_resize )
{
  GtkTreeViewColumn* col =
    gtk_tree_view_get_column( clist_treeview( clist ), column );
  if( col != NULL && auto_resize )
    gtk_tree_view_column_set_sizing( col, GTK_TREE_VIEW_COLUMN_AUTOSIZE );
}

void
gtk_clist_set_column_resizeable( GtkWidget* clist, gint column,
                                 gboolean resizeable )
{
  GtkTreeViewColumn* col =
    gtk_tree_view_get_column( clist_treeview( clist ), column );
  if( col != NULL )
    gtk_tree_view_column_set_resizable( col, resizeable );
}

void
gtk_clist_set_column_max_width( GtkWidget* clist, gint column, gint max_width )
{
  GtkTreeViewColumn* col =
    gtk_tree_view_get_column( clist_treeview( clist ), column );
  if( col != NULL && max_width > 0 )
    gtk_tree_view_column_set_max_width( col, max_width );
}

void
gtk_clist_set_column_visibility( GtkWidget* clist, gint column,
                                 gboolean visible )
{
  GtkTreeViewColumn* col =
    gtk_tree_view_get_column( clist_treeview( clist ), column );
  if( col != NULL )
    gtk_tree_view_column_set_visible( col, visible );
}

void
gtk_clist_column_titles_passive( GtkWidget* clist )
{
  gint n = clist_ncols( clist );
  for( gint c = 0; c < n; c++ )
    {
      GtkTreeViewColumn* col =
        gtk_tree_view_get_column( clist_treeview( clist ), c );
      if( col != NULL )
        gtk_tree_view_column_set_clickable( col, FALSE );
    }
}

void
gtk_clist_column_title_passive( GtkWidget* clist, gint column )
{
  GtkTreeViewColumn* col =
    gtk_tree_view_get_column( clist_treeview( clist ), column );
  if( col != NULL )
    gtk_tree_view_column_set_clickable( col, FALSE );
}

void
gtk_clist_column_title_active( GtkWidget* clist, gint column )
{
  GtkTreeViewColumn* col =
    gtk_tree_view_get_column( clist_treeview( clist ), column );
  if( col != NULL )
    gtk_tree_view_column_set_clickable( col, TRUE );
}

void
gtk_clist_columns_autosize( GtkWidget* clist )
{
  gtk_tree_view_columns_autosize( clist_treeview( clist ) );
}

// ---- No-ops --------------------------------------------------------------

void gtk_clist_set_shadow_type( GtkWidget*, GtkShadowType ) {}
void gtk_clist_set_border( GtkWidget*, GtkShadowType ) {}
void gtk_clist_set_policy( GtkWidget*, GtkPolicyType, GtkPolicyType ) {}
void gtk_clist_freeze( GtkWidget* ) {}
void gtk_clist_thaw( GtkWidget* ) {}

// ---- Filas ---------------------------------------------------------------

static void
clist_set_row_text( GtkWidget* clist, GtkTreeIter* iter, gchar* text[] )
{
  GtkListStore* store = clist_store( clist );
  gint n = clist_ncols( clist );
  for( gint c = 0; c < n; c++ )
    gtk_list_store_set( store, iter, STORE_TEXT( c ),
                        text[c] != NULL ? text[c] : "", -1 );
}

gint
gtk_clist_append( GtkWidget* clist, gchar* text[] )
{
  GtkListStore* store = clist_store( clist );
  GtkTreeIter iter;
  gtk_list_store_append( store, &iter );
  clist_set_row_text( clist, &iter, text );
  return gtk_tree_model_iter_n_children( GTK_TREE_MODEL( store ), NULL ) - 1;
}

gint
gtk_clist_insert( GtkWidget* clist, gint row, gchar* text[] )
{
  GtkListStore* store = clist_store( clist );
  GtkTreeIter iter;
  gtk_list_store_insert( store, &iter, row );
  clist_set_row_text( clist, &iter, text );
  return row;
}

void
gtk_clist_remove( GtkWidget* clist, gint row )
{
  GtkTreeIter iter;
  if( clist_nth_iter( clist, row, &iter ) )
    gtk_list_store_remove( clist_store( clist ), &iter );
}

void
gtk_clist_clear( GtkWidget* clist )
{
  gtk_list_store_clear( clist_store( clist ) );
}

void
gtk_clist_set_text( GtkWidget* clist, gint row, gint column, const gchar* text )
{
  GtkTreeIter iter;
  if( clist_nth_iter( clist, row, &iter ) )
    gtk_list_store_set( clist_store( clist ), &iter,
                        STORE_TEXT( column ),
                        text != NULL ? text : "", -1 );
}

gint
gtk_clist_get_text( GtkWidget* clist, gint row, gint column, gchar** text )
{
  GtkTreeIter iter;
  if( !clist_nth_iter( clist, row, &iter ) )
    {
      if( text != NULL ) *text = NULL;
      return 0;
    }
  gchar* prev = (gchar*) g_object_get_data( G_OBJECT( clist ), "rtb-lasttext" );
  if( prev != NULL ) g_free( prev );

  gchar* value = NULL;
  gtk_tree_model_get( GTK_TREE_MODEL( clist_store( clist ) ), &iter,
                      STORE_TEXT( column ), &value, -1 );
  g_object_set_data( G_OBJECT( clist ), "rtb-lasttext", value );
  if( text != NULL ) *text = value;
  return 1;
}

void
gtk_clist_set_pixbuf( GtkWidget* clist, gint row, gint column,
                      GdkPixbuf* pixbuf )
{
  GtkTreeIter iter;
  if( clist_nth_iter( clist, row, &iter ) )
    gtk_list_store_set( clist_store( clist ), &iter,
                        STORE_PIXBUF, pixbuf, -1 );
}

void
gtk_clist_set_foreground( GtkWidget* clist, gint row, const GdkColor* colour )
{
  GtkTreeIter iter;
  if( !clist_nth_iter( clist, row, &iter ) ) return;
  gint n = clist_ncols( clist );
  GdkRGBA rgba;
  gdkcolor_to_rgba( colour, &rgba );
  gtk_list_store_set( clist_store( clist ), &iter,
                      STORE_FG( n ), &rgba,
                      STORE_FG_SET( n ), TRUE, -1 );
}

void
gtk_clist_set_background( GtkWidget* clist, gint row, const GdkColor* colour )
{
  GtkTreeIter iter;
  if( !clist_nth_iter( clist, row, &iter ) ) return;
  gint n = clist_ncols( clist );
  GdkRGBA rgba;
  gdkcolor_to_rgba( colour, &rgba );
  gtk_list_store_set( clist_store( clist ), &iter,
                      STORE_BG( n ), &rgba,
                      STORE_BG_SET( n ), TRUE, -1 );
}

// ---- Seleccion -----------------------------------------------------------

void
gtk_clist_select_row( GtkWidget* clist, gint row, gint column )
{
  GtkTreeIter iter;
  if( !clist_nth_iter( clist, row, &iter ) ) return;
  GtkTreeSelection* sel =
    gtk_tree_view_get_selection( clist_treeview( clist ) );
  gtk_tree_selection_select_iter( sel, &iter );
}

void
gtk_clist_unselect_row( GtkWidget* clist, gint row, gint column )
{
  GtkTreeIter iter;
  if( !clist_nth_iter( clist, row, &iter ) ) return;
  GtkTreeSelection* sel =
    gtk_tree_view_get_selection( clist_treeview( clist ) );
  gtk_tree_selection_unselect_iter( sel, &iter );
}

// ---- Ordenacion ----------------------------------------------------------

void
gtk_clist_set_compare_func( GtkWidget* clist, GtkClistCompareFunc func )
{
  g_object_set_data( G_OBJECT( clist ), "rtb-compare", (gpointer) func );
}

void
gtk_clist_set_sort_column( GtkWidget* clist, gint column )
{
  g_object_set_data( G_OBJECT( clist ), "rtb-sortcol",
                     GINT_TO_POINTER( column ) );
}

void
gtk_clist_set_sort_type( GtkWidget* clist, GtkSortType type )
{
  g_object_set_data( G_OBJECT( clist ), "rtb-sorttype",
                     GINT_TO_POINTER( type ) );
}

void
gtk_clist_sort( GtkWidget* clist )
{
  GtkClistCompareFunc cmp =
    (GtkClistCompareFunc) g_object_get_data( G_OBJECT( clist ), "rtb-compare" );
  if( cmp == NULL ) return;

  GtkListStore* store = clist_store( clist );
  GtkTreeModel* model = GTK_TREE_MODEL( store );
  gint nrows = gtk_tree_model_iter_n_children( model, NULL );
  if( nrows < 2 ) return;

  gint sort_col = GPOINTER_TO_INT(
    g_object_get_data( G_OBJECT( clist ), "rtb-sortcol" ) );
  GtkSortType stype = (GtkSortType) GPOINTER_TO_INT(
    g_object_get_data( G_OBJECT( clist ), "rtb-sorttype" ) );

  // Se extrae el texto de la columna de orden de cada fila.
  gchar** keys = g_new0( gchar*, nrows );
  gint* order = g_new( gint, nrows );
  for( gint i = 0; i < nrows; i++ )
    {
      GtkTreeIter iter;
      gtk_tree_model_iter_nth_child( model, &iter, NULL, i );
      gtk_tree_model_get( model, &iter, STORE_TEXT( sort_col ), &keys[i], -1 );
      order[i] = i;
    }

  // Ordenacion por insercion (las listas tienen pocas decenas de filas).
  for( gint i = 1; i < nrows; i++ )
    {
      gint oi = order[i];
      gchar* ki = keys[oi];
      gint j = i - 1;
      while( j >= 0 )
        {
          gint c = cmp( keys[order[j]], ki );
          if( stype == GTK_SORT_DESCENDING ) c = -c;
          if( c <= 0 ) break;
          order[j + 1] = order[j];
          j--;
        }
      order[j + 1] = oi;
    }

  gtk_list_store_reorder( store, order );

  for( gint i = 0; i < nrows; i++ )
    g_free( keys[i] );
  g_free( keys );
  g_free( order );
}

// ---- Registro de callbacks ----------------------------------------------

static void
clist_column_clicked( GtkTreeViewColumn* col, gpointer user_data )
{
  GtkWidget* clist = GTK_WIDGET( user_data );
  GtkClistClickColumnFunc cb = (GtkClistClickColumnFunc)
    g_object_get_data( G_OBJECT( clist ), "rtb-click-cb" );
  if( cb == NULL ) return;
  gpointer data = g_object_get_data( G_OBJECT( clist ), "rtb-click-data" );
  gint column = GPOINTER_TO_INT(
    g_object_get_data( G_OBJECT( col ), "rtb-colindex" ) );
  cb( clist, column, data );
}

void
gtk_clist_set_select_callback( GtkWidget* clist,
                               GtkClistSelectFunc select_cb,
                               GtkClistSelectFunc unselect_cb,
                               gpointer data )
{
  g_object_set_data( G_OBJECT( clist ), "rtb-select-cb", (gpointer) select_cb );
  g_object_set_data( G_OBJECT( clist ), "rtb-select-data", data );

  GtkTreeSelection* sel =
    gtk_tree_view_get_selection( clist_treeview( clist ) );
  g_signal_connect( sel, "changed",
                    G_CALLBACK( clist_selection_changed ), clist );
}

void
gtk_clist_set_click_column_callback( GtkWidget* clist,
                                     GtkClistClickColumnFunc cb,
                                     gpointer data )
{
  g_object_set_data( G_OBJECT( clist ), "rtb-click-cb", (gpointer) cb );
  g_object_set_data( G_OBJECT( clist ), "rtb-click-data", data );

  gint n = clist_ncols( clist );
  for( gint c = 0; c < n; c++ )
    {
      GtkTreeViewColumn* col =
        gtk_tree_view_get_column( clist_treeview( clist ), c );
      if( col == NULL ) continue;
      g_object_set_data( G_OBJECT( col ), "rtb-colindex",
                         GINT_TO_POINTER( c ) );
      gtk_tree_view_column_set_clickable( col, TRUE );
      g_signal_connect( col, "clicked",
                        G_CALLBACK( clist_column_clicked ), clist );
    }
}

#endif // NO_GRAPHICS
