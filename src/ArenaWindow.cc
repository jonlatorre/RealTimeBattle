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

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <math.h>

#include "ArenaWindow.h"
#include "IntlDefs.h"
#include "ArenaBase.h"
#include "ArenaController.h"
#include "ArenaRealTime.h"
#include "ControlWindow.h"
#include "Extras.h"
#include "Robot.h"
#include "Shot.h"
#include "String.h"
#include "Structs.h"
#include "Vector2D.h"

extern class ControlWindow* controlwindow_p;

// Convierte un GdkColor (componentes 0..65535) en el color de origen de un
// contexto Cairo (componentes 0..1). GTK 3 no tiene GdkGC ni dibujo inmediato:
// todo el dibujo se hace con Cairo sobre una superficie de respaldo.
static void
set_cairo_source_gdkcolor( cairo_t* cr, const GdkColor& colour )
{
  cairo_set_source_rgb( cr,
                        colour.red   / 65535.0,
                        colour.green / 65535.0,
                        colour.blue  / 65535.0 );
}

// Wrapper para "delete-event": debe devolver TRUE para que GTK no destruya
// la ventana (solo la ocultamos).
static gboolean
arena_delete_event( GtkWidget* widget, GdkEvent* event, gpointer data )
{
  ArenaWindow::hide_window( widget, event, (class ArenaWindow*)data );
  return TRUE;
}

ArenaWindow::ArenaWindow( const int default_width,
                          const int default_height,
                          const int default_x_pos,
                          const int default_y_pos )
{
  pixmap = NULL;

  // The window widget

  window_p = gtk_window_new( GTK_WINDOW_TOPLEVEL );
  gtk_widget_set_name( window_p, "RTB Arena" );

  set_window_title();

  gtk_container_set_border_width( GTK_CONTAINER( window_p ), 12 );

  if( default_width != -1 && default_height != -1 )
    {
      gtk_window_set_default_size( GTK_WINDOW( window_p ),
                                   default_width, default_height );
      gtk_widget_set_size_request( window_p, 185, 120 );
    }
  if( default_x_pos != -1 && default_y_pos != -1 )
    gtk_window_move( GTK_WINDOW( window_p ), default_x_pos, default_y_pos );

  g_signal_connect( G_OBJECT( window_p ), "delete-event",
                    G_CALLBACK( arena_delete_event ),
                    (gpointer) this );

  // Main box

  GtkWidget* vbox = gtk_box_new( GTK_ORIENTATION_VERTICAL, 10 );
  gtk_container_add( GTK_CONTAINER( window_p ), vbox );
  gtk_widget_show( vbox );

  // Zoom buttons

  struct button_t { String label; GCallback func; };

  struct button_t buttons[] = {
    { (String)_(" No Zoom ") + (String)"[0] ",
      (GCallback) ArenaWindow::no_zoom  },
    { (String)_(" Zoom In ") + (String)"[+] ",
      (GCallback) ArenaWindow::zoom_in  },
    { (String)_(" Zoom Out ") + (String)"[-] ",
      (GCallback) ArenaWindow::zoom_out } };

  GtkWidget* button_table = gtk_box_new( GTK_ORIENTATION_HORIZONTAL, 5 );
  gtk_box_set_homogeneous( GTK_BOX( button_table ), TRUE );
  gtk_box_pack_start( GTK_BOX( vbox ), button_table, FALSE, FALSE, 0 );

  for( int i=0; i < 3; i++ )
    {
      GtkWidget* button =
        gtk_button_new_with_label( buttons[i].label.chars() );
      g_signal_connect( G_OBJECT( button ), "clicked",
                        buttons[i].func, (gpointer) this );
      gtk_box_pack_start( GTK_BOX( button_table ), button, TRUE, TRUE, 0 );
      gtk_widget_show( button );
    }

  gtk_widget_show( button_table );

  // Scrolled Window

  scrolled_window = gtk_scrolled_window_new( NULL, NULL );
  gtk_scrolled_window_set_policy( GTK_SCROLLED_WINDOW( scrolled_window ),
                                  GTK_POLICY_ALWAYS, GTK_POLICY_ALWAYS );
  gtk_box_pack_start( GTK_BOX( vbox ), scrolled_window, TRUE, TRUE, 0 );
  gtk_widget_show( scrolled_window );

  // Drawing Area

  drawing_area = gtk_drawing_area_new();
  {
    int da_w = default_width  > 48 ? default_width  - 48 : 137;
    int da_h = default_height > 80 ? default_height - 80 : 40;
    gtk_widget_set_size_request( drawing_area, da_w, da_h );
  }

  g_signal_connect( G_OBJECT( drawing_area ), "draw",
                    G_CALLBACK( ArenaWindow::draw_cb ), (gpointer) this );
  g_signal_connect( G_OBJECT( drawing_area ), "configure-event",
                    G_CALLBACK( ArenaWindow::configure_cb ), (gpointer) this );

  gtk_container_add( GTK_CONTAINER( scrolled_window ), drawing_area );
  gtk_widget_show( drawing_area );

  window_shown = controlwindow_p->is_arenawindow_checked();

  zoom = 1;

  gtk_widget_add_events( window_p, GDK_KEY_PRESS_MASK );
  g_signal_connect( G_OBJECT( window_p ), "key_press_event",
                    G_CALLBACK( ArenaWindow::keyboard_handler ), this );

  gtk_widget_show_now( window_p );

  if( !window_shown )
    gtk_widget_hide( window_p );
}

ArenaWindow::~ArenaWindow()
{
  if( pixmap != NULL )
    cairo_surface_destroy( pixmap );
  gtk_widget_destroy( window_p );
}

void
ArenaWindow::set_window_title()
{
  String title = (String)_("Arena") + "   " +
    the_arena.get_current_arena_filename();
  gtk_window_set_title( GTK_WINDOW( window_p ), title.chars() );
}

void
ArenaWindow::set_window_shown( bool win_shown )
{
  window_shown = win_shown;
}

// Warning: event can be NULL, do not use event!
void
ArenaWindow::hide_window( GtkWidget* widget, GdkEvent* event,
                          class ArenaWindow* arenawindow_p )
{
  if( arenawindow_p->is_window_shown() )
    {
      gtk_widget_hide( arenawindow_p->get_window_p() );
      arenawindow_p->set_window_shown( false );
      if( controlwindow_p->is_arenawindow_checked() )
        {
          GtkWidget* menu_item = controlwindow_p->get_show_arena_menu_item();
          gtk_check_menu_item_set_active( GTK_CHECK_MENU_ITEM( menu_item ), FALSE );
        }
    }
}

void
ArenaWindow::show_window( GtkWidget* widget,
                          class ArenaWindow* arenawindow_p )
{
  if( !arenawindow_p->is_window_shown() )
    {
      gtk_widget_show( arenawindow_p->get_window_p() );
      arenawindow_p->set_window_shown( true );
      arenawindow_p->draw_everything();
    }
}

void
ArenaWindow::clear_area()
{
  if( window_shown && pixmap != NULL )
    {
      cairo_t* cr = cairo_create( pixmap );
      set_cairo_source_gdkcolor( cr, *the_gui.get_bg_gdk_colour_p() );
      cairo_paint( cr );
      cairo_destroy( cr );
      gtk_widget_queue_draw( drawing_area );
    }
}

void
ArenaWindow::draw_everything()
{
  if( window_shown )
    {
      clear_area();

      if( ( gtk_widget_get_allocated_width( scrolled_window ) - 24 != scrolled_window_size[0]) ||
          ( gtk_widget_get_allocated_height( scrolled_window ) - 24 !=  scrolled_window_size[1]) )
        {
          drawing_area_scale_changed();
          return;
        }

      List<Shape>* object_lists;

      object_lists = the_arena.get_object_lists();
      ListIterator<Shape> li;

      // Must begin with innercircles (they are destructive)
      for( int obj_type=WALL; obj_type < LAST_OBJECT_TYPE ; obj_type++)
        {
          for( object_lists[obj_type].first(li); li.ok(); li++ )
            {
              if( !( ( obj_type == MINE || obj_type == COOKIE ) &&
                     !( (Extras*)li() )->is_alive() ) )
                {
                  li()->draw_shape( false );
                }
            }
        }

      draw_moving_objects( false );
    }
}

void
ArenaWindow::draw_moving_objects( const bool clear_objects_first )
{
  if( window_shown )
    {
      List<Shape>* object_lists = the_arena.get_object_lists();
      Robot* robotp;

      if( ( gtk_widget_get_allocated_width( scrolled_window ) - 24 != scrolled_window_size[0]) ||
          ( gtk_widget_get_allocated_height( scrolled_window ) - 24 !=  scrolled_window_size[1]) )
        {
          drawing_area_scale_changed();
          return;
        }

      ListIterator<Shape> li;
      for( object_lists[SHOT].first(li); li.ok(); li++ )
        if( ((Shot*)li())->is_alive() )
          ((Shot*)li())->draw_shape( clear_objects_first );

      ListIterator<Shape> li2;
      for( object_lists[ROBOT].first(li2); li2.ok(); li2++ )
        {
          robotp = (Robot*)li2();

          if( robotp->is_alive() )
            {
              robotp->draw_shape( clear_objects_first );
              robotp->draw_radar_and_cannon();
            }
        }
    }
}

void
ArenaWindow::draw_circle( const Vector2D& center, const double radius,
                          GdkColor& colour, const bool filled )
{
  if( window_shown && pixmap != NULL )
    {
      cairo_t* cr = cairo_create( pixmap );
      set_cairo_source_gdkcolor( cr, colour );

      double r;
      if( ( r = radius * drawing_area_scale ) > 1.0 )
        {
          double cx = boundary2pixel_x( center[0] );
          double cy = boundary2pixel_y( center[1] );
          cairo_new_path( cr );
          cairo_arc( cr, cx, cy, r, 0.0, 2.0 * M_PI );
          if( filled )
            cairo_fill( cr );
          else
            {
              cairo_set_line_width( cr, 1.0 );
              cairo_stroke( cr );
            }
        }
      else
        {
          double px = boundary2pixel_x( center[0] );
          double py = boundary2pixel_y( center[1] );
          cairo_rectangle( cr, px, py, 1.0, 1.0 );
          cairo_fill( cr );
        }
      cairo_destroy( cr );
      gtk_widget_queue_draw( drawing_area );
    }
}

void
ArenaWindow::draw_arc( const Vector2D& center,
                       const double inner_radius, const double outer_radius,
                       const double angle1, const double angle2,
                       GdkColor& colour )
{
  if( window_shown && pixmap != NULL )
    {
      double r = 0.5 * ( outer_radius + inner_radius );
      int box_size = (int)( r*2.0*drawing_area_scale + 0.5 );

      cairo_t* cr = cairo_create( pixmap );
      set_cairo_source_gdkcolor( cr, colour );

      if( box_size >= 2.0 )
        {
          double cx = boundary2pixel_x( center[0] );
          double cy = boundary2pixel_y( center[1] );
          double line_width =
            ( outer_radius - inner_radius ) * drawing_area_scale;
          if( line_width < 1.0 ) line_width = 1.0;
          cairo_set_line_width( cr, line_width );
          // Cairo mide los angulos en sentido horario (y hacia abajo); el
          // codigo original usa angulos matematicos (antihorario), por eso
          // se niegan.
          cairo_new_path( cr );
          cairo_arc_negative( cr, cx, cy, r * drawing_area_scale,
                              -angle1, -angle2 );
          cairo_stroke( cr );
        }
      else
        {
          cairo_rectangle( cr, boundary2pixel_x( center[0] ),
                           boundary2pixel_y( center[1] ), 1.0, 1.0 );
          cairo_fill( cr );
        }
      cairo_destroy( cr );
      gtk_widget_queue_draw( drawing_area );
    }
}


void
ArenaWindow::draw_rectangle( const Vector2D& start,
                             const Vector2D& end,
                             GdkColor& colour,
                             const bool filled )
{
  if( window_shown && pixmap != NULL )
    {
      cairo_t* cr = cairo_create( pixmap );
      set_cairo_source_gdkcolor( cr, colour );

      cairo_rectangle( cr,
                       boundary2pixel_x( start[0] ),
                       boundary2pixel_y( end[1] ),
                       boundary2pixel_x( end[0] - start[0] ),
                       boundary2pixel_y( end[1] - start[1] ) );
      if( filled )
        cairo_fill( cr );
      else
        {
          cairo_set_line_width( cr, 1.0 );
          cairo_stroke( cr );
        }
      cairo_destroy( cr );
      gtk_widget_queue_draw( drawing_area );
    }
}

void
ArenaWindow::draw_line( const Vector2D& start, const Vector2D& direction,
                        const double length, const double thickness,
                        GdkColor& colour )
{
  if( window_shown && pixmap != NULL )
    {
      Vector2D vector_points[4];

      cairo_t* cr = cairo_create( pixmap );
      set_cairo_source_gdkcolor( cr, colour );

      Vector2D line_thick = unit( direction );
      line_thick = rotate90( line_thick );
      line_thick *= thickness;
      vector_points[0] = start + line_thick;
      vector_points[1] = start - line_thick;
      vector_points[2] = start - line_thick + direction * length;
      vector_points[3] = start + line_thick + direction * length;

      cairo_new_path( cr );
      cairo_move_to( cr, boundary2pixel_x( vector_points[0][0] ),
                     boundary2pixel_y( vector_points[0][1] ) );
      for( int i=1; i<4; i++ )
        cairo_line_to( cr, boundary2pixel_x( vector_points[i][0] ),
                       boundary2pixel_y( vector_points[i][1] ) );
      cairo_close_path( cr );
      cairo_fill( cr );

      cairo_destroy( cr );
      gtk_widget_queue_draw( drawing_area );
    }
}

void
ArenaWindow::draw_line( const Vector2D& start, const Vector2D& direction,
                        const double length, GdkColor& colour )
{
  if( window_shown && pixmap != NULL )
    {
      cairo_t* cr = cairo_create( pixmap );
      set_cairo_source_gdkcolor( cr, colour );

      Vector2D end_point = start + length * direction;

      cairo_set_line_width( cr, 1.0 );
      cairo_new_path( cr );
      cairo_move_to( cr, boundary2pixel_x( start[0] ),
                     boundary2pixel_y( start[1] ) );
      cairo_line_to( cr, boundary2pixel_x( end_point[0] ),
                     boundary2pixel_y( end_point[1] ) );
      cairo_stroke( cr );

      cairo_destroy( cr );
      gtk_widget_queue_draw( drawing_area );
    }
}

void
ArenaWindow::drawing_area_scale_changed( const bool change_da_value )
{
  if( window_shown )
    {
      int width = gtk_widget_get_allocated_width( scrolled_window ) - 24;
      int height = gtk_widget_get_allocated_height( scrolled_window ) - 24;
      scrolled_window_size = Vector2D( (double)width,
                                       (double)height );
      double w = (double)( width * zoom );
      double h = (double)( height * zoom );
      double bw = the_arena.get_boundary()[1][0] -
        the_arena.get_boundary()[0][0];
      double bh = the_arena.get_boundary()[1][1] -
        the_arena.get_boundary()[0][1];
      if( w / bw >= h / bh )
        {
          drawing_area_scale = h / bh;
          w = drawing_area_scale * bw;
        }
      else
        {
          drawing_area_scale = w / bw;
          h = drawing_area_scale * bh;
        }

      gtk_widget_set_size_request( drawing_area,
                                   (int)w > 0 ? (int)w : 1,
                                   (int)h > 0 ? (int)h : 1 );
      if( change_da_value )
        {
          GtkAdjustment* hadj = gtk_scrolled_window_get_hadjustment
            ( (GtkScrolledWindow*) scrolled_window );
          double hpage = gtk_adjustment_get_page_size( hadj );
          gtk_adjustment_set_value
            ( hadj,
              ( ( gtk_adjustment_get_value( hadj ) + hpage / 2 ) /
                ( gtk_adjustment_get_upper( hadj ) -
                  gtk_adjustment_get_lower( hadj ) ) ) *
              (int)w - hpage / 2 );
          GtkAdjustment* vadj = gtk_scrolled_window_get_vadjustment
            ( (GtkScrolledWindow*) scrolled_window );
          double vpage = gtk_adjustment_get_page_size( vadj );
          gtk_adjustment_set_value
            ( vadj,
              ( ( gtk_adjustment_get_value( vadj ) + vpage / 2 ) /
                ( gtk_adjustment_get_upper( vadj ) -
                  gtk_adjustment_get_lower( vadj ) ) ) *
              (int)h - vpage / 2 );
        }
      else
        draw_everything();
    }
}

// Warning! Do not use widget, may be NULL or undefined
void
ArenaWindow::no_zoom( GtkWidget* widget, class ArenaWindow* arenawindow_p )
{
  arenawindow_p->set_zoom( 1 );
  arenawindow_p->drawing_area_scale_changed();
}

// Warning! Do not use widget, may be NULL or undefined
void
ArenaWindow::zoom_in( GtkWidget* widget, class ArenaWindow* arenawindow_p )
{
  int z = arenawindow_p->get_zoom();
  z++;
  arenawindow_p->set_zoom( z );
  arenawindow_p->drawing_area_scale_changed( true );
}

// Warning! Do not use widget, may be NULL or undefined
void
ArenaWindow::zoom_out( GtkWidget* widget, class ArenaWindow* arenawindow_p )
{
  int z = arenawindow_p->get_zoom();
  if( z > 1 )
    z--;
  arenawindow_p->set_zoom( z );
  arenawindow_p->drawing_area_scale_changed( true );
}

gint
ArenaWindow::keyboard_handler( GtkWidget* widget, GdkEventKey *event,
                               class ArenaWindow* arenawindow_p )
{
  switch (event->keyval)
    {
    case GDK_KEY_plus:
    case GDK_KEY_KP_Add:
      zoom_in( NULL, arenawindow_p );
      break;

    case GDK_KEY_minus:
    case GDK_KEY_KP_Subtract:
      zoom_out( NULL, arenawindow_p );
      break;

    case GDK_KEY_0:
    case GDK_KEY_KP_0:
    case GDK_KEY_KP_Insert:
      no_zoom( NULL, arenawindow_p );
      break;

    default:
      return FALSE;
    }

  g_signal_stop_emission_by_name( G_OBJECT(widget), "key_press_event" );
  return FALSE;
}

// (Re)crea la superficie de respaldo cuando cambia el tamano del area de dibujo.
gint
ArenaWindow::configure_cb( GtkWidget* widget, GdkEventConfigure* event,
                           class ArenaWindow* arenawindow_p )
{
  if( arenawindow_p->pixmap != NULL )
    cairo_surface_destroy( arenawindow_p->pixmap );

  int width  = gtk_widget_get_allocated_width( widget );
  int height = gtk_widget_get_allocated_height( widget );

  arenawindow_p->pixmap =
    gdk_window_create_similar_surface( gtk_widget_get_window( widget ),
                                       CAIRO_CONTENT_COLOR,
                                       width, height );

  cairo_t* cr = cairo_create( arenawindow_p->pixmap );
  set_cairo_source_gdkcolor( cr, *the_gui.get_bg_gdk_colour_p() );
  cairo_paint( cr );
  cairo_destroy( cr );

  if( the_arena_controller.is_started() )
    arenawindow_p->draw_everything();

  return TRUE;
}

gint
ArenaWindow::draw_cb( GtkWidget* widget, cairo_t* cr,
                      class ArenaWindow* arenawindow_p )
{
  if( arenawindow_p->pixmap != NULL )
    {
      cairo_set_source_surface( cr, arenawindow_p->pixmap, 0, 0 );
      cairo_paint( cr );
    }
  return FALSE;
}
