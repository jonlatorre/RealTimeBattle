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

#include <gtk/gtk.h>

#include "ControlWindow.h"
#include "IntlDefs.h"
#include "ArenaWindow.h"
#include "MessageWindow.h"
#include "ScoreWindow.h" 
#include "StatisticsWindow.h" 
#include "Gui.h"
#include "Dialog.h"
#include "ArenaController.h"
#include "ArenaRealTime.h"
#include "ArenaReplay.h"
#include "Robot.h"
#include "Options.h"
#include "String.h"

extern class ControlWindow* controlwindow_p;

ControlWindow::ControlWindow( const int default_width,
                              const int default_height,
                              const int default_x_pos,
                              const int default_y_pos )
{
  // The window widget

  window_p = gtk_window_new( GTK_WINDOW_TOPLEVEL );
  gtk_widget_set_name( window_p, "RTB Control" );
  gtk_window_set_resizable( GTK_WINDOW( window_p ), FALSE );

  set_window_title( "RealTimeBattle" );

  gtk_container_set_border_width( GTK_CONTAINER( window_p ), 12 );

  if( default_width != -1 && default_height != -1 )
    gtk_widget_set_size_request( window_p, default_width, default_height );
  if( default_x_pos != -1 && default_y_pos != -1 )
    gtk_window_move( GTK_WINDOW( window_p ), default_x_pos, default_y_pos );

  g_signal_connect( G_OBJECT( window_p ), "delete_event",
                      (GCallback) ControlWindow::delete_event_occured,
                      (gpointer) this );

  // Main boxes

  window_hbox = gtk_box_new( GTK_ORIENTATION_HORIZONTAL, 10 );
  gtk_container_add( GTK_CONTAINER( window_p ), window_hbox );
  gtk_widget_show( window_hbox );

  GtkWidget* vbox = gtk_box_new( GTK_ORIENTATION_VERTICAL, 10 );
  gtk_container_add( GTK_CONTAINER( window_hbox ), vbox );
  gtk_widget_show( vbox );

  // Buttons for all modes

  struct button_t { String label; GCallback func; int pack; };

  struct button_t buttons[] = {
    { (String)_(" New Tournament "), 
      (GCallback) ControlWindow::new_tournament    , TRUE  },
    { (String)_(" Replay Tournament "), 
      (GCallback) ControlWindow::replay_tournament , TRUE  },
    { (String)_(" Pause "),
      (GCallback) ControlWindow::pause             , TRUE  },
    { (String)_(" End "),
      (GCallback) ControlWindow::end_clicked       , TRUE  },
    { (String)_(" Options "),
      (GCallback) ControlWindow::options_clicked   , TRUE  },
    { (String)_(" Statistics "),
      (GCallback) ControlWindow::statistics_clicked, TRUE  },
    { (String)_("         Quit         "),
      (GCallback) ControlWindow::quit_rtb          , FALSE } };

  GtkWidget* button_hbox[3] = { NULL,NULL,NULL };
  int hbox_index = -1;
  for(int i = 0;i < 7; i++)
    {
      if( i == 0 || i == 4 || i == 6 )
        {
          hbox_index++;
          button_hbox[hbox_index] = gtk_box_new( GTK_ORIENTATION_HORIZONTAL, 10 );
          gtk_box_pack_start( GTK_BOX( vbox ), button_hbox[hbox_index],
                              FALSE, FALSE, 0);
          gtk_widget_show( button_hbox[hbox_index] );
        }
      GtkWidget* button =
        gtk_button_new_with_label( buttons[i].label.chars() );
      g_signal_connect( G_OBJECT( button ), "clicked",
                          (GCallback) buttons[i].func,
                          (gpointer) this );
      gtk_box_pack_start( GTK_BOX( button_hbox[hbox_index] ), button,
                          TRUE, buttons[i].pack , 0 );
      gtk_widget_show( button );
    }

  struct button_t menu_items_data[] = {
    { (String)_("Show arena window"),
      (GCallback) ControlWindow::arena_window_toggle, TRUE  },
    { (String)_("Show message window"),
      (GCallback) ControlWindow::message_window_toggle, TRUE  },
    { (String)_("Show score window"),
      (GCallback) ControlWindow::score_window_toggle, TRUE  } };
  

  // GtkOptionMenu was removed in GTK 3. Use a GtkMenuBar with a submenu
  // holding GtkCheckMenuItem toggles (arena/message/score windows).
  GtkWidget* menubar = gtk_menu_bar_new();
  GtkWidget* menu = gtk_menu_new();
  GtkWidget* root_menu_item =
    gtk_menu_item_new_with_label( _("Show windows") );
  gtk_menu_item_set_submenu( GTK_MENU_ITEM( root_menu_item ), menu );
  gtk_menu_shell_append( GTK_MENU_SHELL( menubar ), root_menu_item );
  gtk_widget_show( root_menu_item );

  for( int i = 0;i < 3; i++ )
    {
      GtkWidget* menu_item =
        gtk_check_menu_item_new_with_label( menu_items_data[i].label.chars() );
      g_signal_connect( G_OBJECT( menu_item ), "toggled",
                          menu_items_data[i].func, (gpointer) this );
      gtk_menu_shell_append( GTK_MENU_SHELL( menu ), menu_item );
      gtk_check_menu_item_set_active( GTK_CHECK_MENU_ITEM( menu_item ), TRUE );
      gtk_widget_show( menu_item );

      switch( i )
        {
        case 0:
          show_arena_menu_item = menu_item;
          break;
        case 1:
          show_message_menu_item = menu_item;
          break;
        case 2:
          show_score_menu_item = menu_item;
          break;
        }
    }

  gtk_box_pack_start( GTK_BOX( button_hbox[1] ), menubar, TRUE, TRUE, 0 );
  gtk_widget_show( menubar );

  vseparator = NULL;
  extra_vbox = NULL;
  filesel = NULL;

  remove_replay_widgets();
  gtk_widget_show( window_p );
}

void
ControlWindow::remove_replay_widgets()
{
  if( the_arena_controller.game_mode == ArenaBase::DEBUG_MODE )
    display_debug_widgets();
  else
    clear_extra_widgets();
}

void
ControlWindow::clear_extra_widgets()
{
  if( extra_vbox != NULL ) gtk_widget_destroy( extra_vbox );
  if( vseparator != NULL ) gtk_widget_destroy( vseparator );

  extra_vbox = NULL;
  vseparator = NULL;

  displayed = NO_WIDGETS;
}

void
ControlWindow::display_debug_widgets()
{
  clear_extra_widgets();
  
  vseparator = gtk_vseparator_new();
  gtk_box_pack_start( GTK_BOX (window_hbox), vseparator, FALSE, FALSE, 0 );
  gtk_widget_show( vseparator );

  extra_vbox = gtk_box_new( GTK_ORIENTATION_VERTICAL, 10 );
  gtk_container_add( GTK_CONTAINER( window_hbox ), extra_vbox );
  gtk_widget_show( extra_vbox );

  struct button_t { String label; GCallback func; int pack; };
  struct button_t debug_buttons[] = {
    { (String)_(" Step "), 
      (GCallback) ControlWindow::step      , TRUE  },
    { (String)_(" End Game "), 
      (GCallback) ControlWindow::end_game  , TRUE  },
    { (String)_(" Kill Marked Robot "), 
      (GCallback) ControlWindow::kill_robot, TRUE  } };

  GtkWidget* button_hbox = NULL;
  for(int i = 0;i < 3; i++)
    {
      if( i == 0 || i == 2 )
        {
          button_hbox = gtk_box_new( GTK_ORIENTATION_HORIZONTAL, 10 );
          gtk_box_pack_start( GTK_BOX( extra_vbox ), button_hbox,
                              FALSE, FALSE, 0);
          gtk_widget_show( button_hbox );
        }
      GtkWidget* button = 
        gtk_button_new_with_label( debug_buttons[i].label.chars() );
      g_signal_connect( G_OBJECT( button ), "clicked",
                          (GCallback) debug_buttons[i].func,
                          (gpointer) NULL );
      gtk_box_pack_start( GTK_BOX( button_hbox ), button,
                          TRUE, debug_buttons[i].pack , 0);
      gtk_widget_show( button );
    }

  button_hbox = gtk_box_new( GTK_ORIENTATION_HORIZONTAL, 10 );
  gtk_box_pack_start( GTK_BOX( extra_vbox ), button_hbox,
                      FALSE, FALSE, 0);
  gtk_widget_show( button_hbox );

  GtkWidget* label = gtk_label_new( _(" Debug Level: ") );
  gtk_box_pack_start( GTK_BOX( button_hbox ), label, TRUE, FALSE, 0 );
  gtk_widget_show( label );

  GtkAdjustment* adj =
    (GtkAdjustment*) gtk_adjustment_new( the_arena_controller.debug_level, 0,
                                         max_debug_level, 1, 1, 0 );

  debug_level = gtk_spin_button_new( adj, 0, 0 );
  g_signal_connect( G_OBJECT( adj ), "value_changed",
                      (GCallback) change_debug_level,
                      (gpointer) this );
  gtk_box_pack_start( GTK_BOX( button_hbox ), debug_level, TRUE, FALSE, 0 );
  gtk_widget_show( debug_level );

  displayed = DEBUG_WIDGETS;
}

void
ControlWindow::display_replay_widgets()
{
  clear_extra_widgets();

  vseparator = gtk_vseparator_new();
  gtk_box_pack_start( GTK_BOX (window_hbox), vseparator, FALSE, FALSE, 0 );
  gtk_widget_show( vseparator );

  extra_vbox = gtk_box_new( GTK_ORIENTATION_VERTICAL, 10 );
  gtk_container_add( GTK_CONTAINER( window_hbox ), extra_vbox );
  gtk_widget_show( extra_vbox );

  GtkWidget* hbox = gtk_box_new( GTK_ORIENTATION_HORIZONTAL, 10 );
  gtk_box_pack_start( GTK_BOX( extra_vbox ), hbox, FALSE, FALSE, 0 );
  gtk_widget_show( hbox );

  current_replay_time_adjustment =
    (GtkAdjustment*) gtk_adjustment_new ( 0.0, 0.0,
                                          replay_arena.get_length_of_current_game(),
                                          0.1, 1.0, 1.0 );

  g_signal_connect( G_OBJECT( current_replay_time_adjustment ), "value_changed",
                      (GCallback) change_current_replay_time,
                      (gpointer) this );

  time_control =
    gtk_hscale_new( GTK_ADJUSTMENT( current_replay_time_adjustment ) );
  gtk_widget_set_size_request( GTK_WIDGET( time_control ), 150, 30 );
  gtk_scale_set_value_pos( GTK_SCALE( time_control ), GTK_POS_TOP);
  gtk_scale_set_digits( GTK_SCALE( time_control ), 0 );
  gtk_scale_set_draw_value( GTK_SCALE( time_control ), TRUE );
  gtk_box_pack_start( GTK_BOX( hbox ), time_control, TRUE, TRUE, 0 );
  gtk_widget_show( time_control );

  char* rew_xpm[13] =
  { "18 10 2 1",
    "       c None",
    "x      c #000000000000",
    "       xx       xx",
    "     xxxx     xxxx",  
    "   xxxxxx   xxxxxx",
    " xxxxxxxx xxxxxxxx",
    "xxxxxxxxxxxxxxxxxx",
    "xxxxxxxxxxxxxxxxxx",
    " xxxxxxxx xxxxxxxx",
    "   xxxxxx   xxxxxx",
    "     xxxx     xxxx",
    "       xx       xx" };
  char* ffw_xpm[13] =
  { "18 10 2 1",
    "       c None",
    "x      c #000000000000",
    "xx       xx       ",
    "xxxx     xxxx     ",  
    "xxxxxx   xxxxxx   ",
    "xxxxxxxx xxxxxxxx ",
    "xxxxxxxxxxxxxxxxxx",
    "xxxxxxxxxxxxxxxxxx",
    "xxxxxxxx xxxxxxxx ",
    "xxxxxx   xxxxxx   ",
    "xxxx     xxxx     ",
    "xx       xx       " };

  struct button_t
  {
    char** xpm;
    String label;
    GCallback clicked_func;
    GCallback pressed_func;
    GCallback released_func;
  };

  struct button_t replay_buttons[] = {
    { rew_xpm, (String)"",
      (GCallback) ControlWindow::dummy,
      (GCallback) ControlWindow::rewind_pressed,
      (GCallback) ControlWindow::rewind_released },
    { ffw_xpm, (String)"", 
      (GCallback) ControlWindow::dummy,
      (GCallback) ControlWindow::fast_forward_pressed,
      (GCallback) ControlWindow::fast_forward_released },
    { NULL, (String)_(" Step forward "), 
      (GCallback) ControlWindow::step_forward,
      (GCallback) ControlWindow::dummy,
      (GCallback) ControlWindow::dummy },
    { NULL, (String)_(" Step backward "), 
      (GCallback) ControlWindow::step_backward,
      (GCallback) ControlWindow::dummy,
      (GCallback) ControlWindow::dummy },
    { NULL, (String)_(" Next Game "), 
      (GCallback) ControlWindow::next_game,
      (GCallback) ControlWindow::dummy,
      (GCallback) ControlWindow::dummy },
    { NULL, (String)_(" Prev Game "), 
      (GCallback) ControlWindow::prev_game,
      (GCallback) ControlWindow::dummy,
      (GCallback) ControlWindow::dummy },
    { NULL, (String)_(" Next Seq "), 
      (GCallback) ControlWindow::next_seq,
      (GCallback) ControlWindow::dummy,
      (GCallback) ControlWindow::dummy },
    { NULL, (String)_(" Prev Seq "), 
      (GCallback) ControlWindow::prev_seq,
      (GCallback) ControlWindow::dummy,
      (GCallback) ControlWindow::dummy } };

  GtkWidget* button_hbox = NULL;

  for(int i = 0;i < 8; i++)
    {
      if( i == 0 || i == 4 )
        {
          button_hbox = gtk_box_new( GTK_ORIENTATION_HORIZONTAL, 10 );
          gtk_box_pack_start( GTK_BOX( extra_vbox ), button_hbox,
                              FALSE, FALSE, 0);
          gtk_widget_show( button_hbox );
        }
      GtkWidget* button_w;
      if( replay_buttons[i].xpm != NULL )
        {
          button_w = gtk_button_new();
          GdkPixbuf* pixbuf =
            gdk_pixbuf_new_from_xpm_data( (const char**) replay_buttons[i].xpm );
          GtkWidget* pixmap_widget = gtk_image_new_from_pixbuf( pixbuf );
          gtk_widget_show( pixmap_widget );
          gtk_container_add( GTK_CONTAINER( button_w ), pixmap_widget );
          gtk_widget_set_size_request( button_w, 32, 20 );
        }
      else
        button_w = 
          gtk_button_new_with_label( replay_buttons[i].label.chars() );

      g_signal_connect( G_OBJECT( button_w ), "clicked",
                          (GCallback) replay_buttons[i].clicked_func,
                          (gpointer) NULL );
      g_signal_connect( G_OBJECT( button_w ), "pressed",
                          (GCallback) replay_buttons[i].pressed_func,
                          (gpointer) NULL );
      g_signal_connect( G_OBJECT( button_w ), "released",
                          (GCallback) replay_buttons[i].released_func,
                          (gpointer) NULL );
      gtk_box_pack_start( GTK_BOX( button_hbox ), button_w,
                          TRUE, TRUE , 0);
      gtk_widget_show( button_w );
    }

  displayed = REPLAY_WIDGETS;
}

void
ControlWindow::change_time_limitations()
{
  if( displayed == REPLAY_WIDGETS )
    {
      // Possible memory leak: Do not know how to destroy old adjustment
      // possibly destroyed by gtk_range_set_adjustment()
      current_replay_time_adjustment =
        (GtkAdjustment*) gtk_adjustment_new ( 0.0, 0.0,
                                              replay_arena.get_length_of_current_game(),
                                              0.1, 1.0, 1.0 );
      g_signal_connect( G_OBJECT( current_replay_time_adjustment ), "value_changed",
                          (GCallback) change_current_replay_time,
                          (gpointer) this );
      gtk_range_set_adjustment( GTK_RANGE( time_control ),
                                current_replay_time_adjustment );
    }
}

ControlWindow::~ControlWindow()
{
  gtk_widget_destroy( window_p );
}

void
ControlWindow::set_window_title( const String& text)
{
  String title = text;
  gtk_window_set_title( GTK_WINDOW( window_p ), title.chars() );
}

void
ControlWindow::delete_event_occured( GtkWidget* widget, GdkEvent* event,
                                     class ControlWindow* cw_p )
{
  Quit();
}

void
ControlWindow::quit_rtb( GtkWidget* widget,
                         class ControlWindow* cw_p )
{
  Quit();
}

void
ControlWindow::pause( GtkWidget* widget, class ControlWindow* cw_p )
{
  if( the_arena_controller.is_started() )
    the_arena.pause_game_toggle();
}

void
ControlWindow::step( GtkWidget* widget, gpointer data )
{
  if( the_arena_controller.is_started() )
    the_arena.step_paused_game();
}

void
ControlWindow::end_game( GtkWidget* widget, gpointer data )
{
  if( the_arena_controller.is_started() )
    if( the_arena.get_state() != NOT_STARTED &&
        the_arena.get_state() != FINISHED )
      the_arena.end_game();
}

void
ControlWindow::kill_robot( GtkWidget* widget, gpointer data )
{
  if( the_arena_controller.is_started() )
    if(the_arena.get_state() == GAME_IN_PROGRESS || 
       the_arena.get_state() == PAUSED )
      {
        Robot* robotp = the_gui.get_scorewindow_p()->get_selected_robot();
        if( robotp != NULL )
          robotp->die();
      }
}

void
ControlWindow::change_debug_level( GtkAdjustment *adj,
                                   class ControlWindow* cw_p )
{
  if( the_arena_controller.is_started() )
    the_arena.set_debug_level
      ( gtk_spin_button_get_value_as_int
        ( GTK_SPIN_BUTTON( cw_p->debug_level ) ) );
}

void
ControlWindow::new_tournament( GtkWidget* widget,
                               class ControlWindow* cw_p )
{
  the_gui.open_starttournamentwindow();
}

void
ControlWindow::replay_tournament( GtkWidget* widget,
                                  class ControlWindow* cw_p )
{
  if( the_arena_controller.is_started() &&
      ( the_arena.get_state() != NOT_STARTED &&
        the_arena.get_state() != FINISHED ) )
    {
      String info_text = _("This action will kill the current tournament.\n"
                           "Do you want to do that?");
      List<String> string_list;
      string_list.insert_last( new String( _("Yes") ) );
      string_list.insert_last( new String( _("No")  ) );
      Dialog( info_text, string_list,
              (DialogFunction) ControlWindow::kill_and_open_filesel );
    }
    else
      cw_p->open_replay_filesel();
}

void
ControlWindow::open_replay_filesel()
{
  if( filesel == NULL )
    {
      // GtkFileSelection was removed in GTK 3; use GtkFileChooserDialog.
      filesel = gtk_file_chooser_dialog_new
        ( _("Choose a log file to replay"), GTK_WINDOW( window_p ),
          GTK_FILE_CHOOSER_ACTION_OPEN, NULL, NULL );
      GtkWidget* cancel_button =
        gtk_dialog_add_button( GTK_DIALOG( filesel ),
                               _("Cancel"), GTK_RESPONSE_CANCEL );
      GtkWidget* ok_button =
        gtk_dialog_add_button( GTK_DIALOG( filesel ),
                               _("OK"), GTK_RESPONSE_ACCEPT );
      g_signal_connect( G_OBJECT( filesel ), "destroy",
                          (GCallback) ControlWindow::destroy_filesel,
                          (gpointer) this );
      g_signal_connect( G_OBJECT( cancel_button ), "clicked",
                          (GCallback) ControlWindow::destroy_filesel,
                          (gpointer) this );
      g_signal_connect( G_OBJECT( ok_button ), "clicked",
                          (GCallback) ControlWindow::replay,
                          (gpointer) this );
      gtk_widget_show( filesel );
    }
}

void
ControlWindow::kill_and_open_filesel( int result )
{
  if( the_arena_controller.is_started() && result == 1 )
    {
      the_arena.interrupt_tournament();
      controlwindow_p->open_replay_filesel();
    }
}


void
ControlWindow::arena_window_toggle( GtkWidget* widget,
                                    class ControlWindow* cw_p )
{
  bool active = gtk_check_menu_item_get_active( GTK_CHECK_MENU_ITEM( widget ) );

  if( the_gui.is_arenawindow_up() )
    {
      if( active )
        the_gui.get_arenawindow_p()->
          show_window( the_gui.get_arenawindow_p()->get_window_p(),
                       the_gui.get_arenawindow_p() );
      else
        the_gui.get_arenawindow_p()->
          hide_window( the_gui.get_arenawindow_p()->get_window_p(),
                       NULL, the_gui.get_arenawindow_p() );
    }
}

bool
ControlWindow::is_arenawindow_checked()
{
  return gtk_check_menu_item_get_active( GTK_CHECK_MENU_ITEM( show_arena_menu_item ) );
}

void
ControlWindow::message_window_toggle( GtkWidget* widget,
                                      class ControlWindow* cw_p )
{
  bool active = gtk_check_menu_item_get_active( GTK_CHECK_MENU_ITEM( widget ) );

  if( the_gui.is_messagewindow_up() )
    {
      if( active )
        the_gui.get_messagewindow_p()->
          show_window( the_gui.get_messagewindow_p()->get_window_p(),
                       the_gui.get_messagewindow_p() );
      else
        the_gui.get_messagewindow_p()->
          hide_window( the_gui.get_messagewindow_p()->get_window_p(),
                       NULL, the_gui.get_messagewindow_p() );
    }
}

bool
ControlWindow::is_messagewindow_checked()
{
  return gtk_check_menu_item_get_active( GTK_CHECK_MENU_ITEM( show_message_menu_item ) );
}

void
ControlWindow::score_window_toggle( GtkWidget* widget,
                                    class ControlWindow* cw_p )
{
  bool active = gtk_check_menu_item_get_active( GTK_CHECK_MENU_ITEM( widget ) );

  if( the_gui.is_scorewindow_up() )
    {
      if( active )
        the_gui.get_scorewindow_p()->
          show_window( the_gui.get_scorewindow_p()->get_window_p(),
                       the_gui.get_scorewindow_p() );
      else
        the_gui.get_scorewindow_p()->
          hide_window( the_gui.get_scorewindow_p()->get_window_p(),
                       NULL, the_gui.get_scorewindow_p() );
    }
}

bool
ControlWindow::is_scorewindow_checked()
{
  return gtk_check_menu_item_get_active( GTK_CHECK_MENU_ITEM( show_score_menu_item ) );
}

void
ControlWindow::replay( GtkWidget* widget,
                       class ControlWindow* cw_p )
{
  char* chosen =
    gtk_file_chooser_get_filename
    ( GTK_FILE_CHOOSER( cw_p->get_filesel() ) );
  String filename = ( chosen != NULL ) ? (String) chosen : (String)"";
  if( chosen != NULL )
    g_free( chosen );

  destroy_filesel( cw_p->get_filesel(), cw_p );

  if( filename.get_length() == 0 ||
      filename[filename.get_length() - 1] == '/' )
    return;  // no file is selected

  the_arena_controller.replay_filename = filename;
  the_arena_controller.start_replay_arena();
}

void
ControlWindow::destroy_filesel( GtkWidget* widget,
                                class ControlWindow* cw_p )
{
  gtk_widget_destroy( cw_p->get_filesel() );
  cw_p->set_filesel( NULL );
}

void
ControlWindow::end_clicked( GtkWidget* widget, gpointer data )
{
  if( the_arena_controller.is_started() )
    if( the_arena.get_state() != NOT_STARTED &&
        the_arena.get_state() != FINISHED )
      {
        String info_text = _("This action will kill the current tournament.\n"
                             "Do you want to do that?");
        List<String> string_list;
        string_list.insert_last( new String( _("Yes") ) );
        string_list.insert_last( new String( _("No")  ) );
        Dialog( info_text, string_list,
                (DialogFunction) ControlWindow::end_tournament );
      }
}

void
ControlWindow::end_tournament( int result )
{
  if( the_arena_controller.is_started() && result == 1 )
    the_arena.interrupt_tournament();
}

void
ControlWindow::options_clicked( GtkWidget* widget,
                                class ControlWindow* cw_p )
{
  the_opts.open_optionswindow();
}

void
ControlWindow::statistics_clicked( GtkWidget* widget,
                                   class ControlWindow* cw_p )
{
  the_gui.open_statisticswindow();
}

void
ControlWindow::rewind_pressed( GtkWidget* widget,
                               class ControlWindow* cw_p )
{
  //  cout << "rewinding ... " << endl;
  replay_arena.change_speed( false, true );
}

void
ControlWindow::rewind_released( GtkWidget* widget,
                                class ControlWindow* cw_p )
{
  //  cout << "until released" << endl;
  replay_arena.change_speed( false, false );
}

void
ControlWindow::fast_forward_pressed( GtkWidget* widget,
                                     class ControlWindow* cw_p )
{
  //  cout << "forwarding ... " << endl;
  replay_arena.change_speed( true, true );
}

void
ControlWindow::fast_forward_released( GtkWidget* widget,
                                      class ControlWindow* cw_p )
{
  //  cout << "until released" << endl;
  replay_arena.change_speed( true, false );
}

void
ControlWindow::step_forward( GtkWidget* widget,
                             class ControlWindow* cw_p )
{
  //  cout << "Stepping forward" << endl;
  replay_arena.step_forward_or_backward( true );  
}

void
ControlWindow::step_backward( GtkWidget* widget,
                              class ControlWindow* cw_p )
{
  //  cout << "Stepping backward" << endl;
  replay_arena.step_forward_or_backward( false );
}

void
ControlWindow::next_game( GtkWidget* widget,
                          class ControlWindow* cw_p )
{
  replay_arena.change_game( 1, 0 );
}

void
ControlWindow::prev_game( GtkWidget* widget,
                          class ControlWindow* cw_p )
{
  replay_arena.change_game( -1, 0 );
}

void
ControlWindow::next_seq( GtkWidget* widget,
                         class ControlWindow* cw_p )
{
  replay_arena.change_game( 0, 1 );
}

void
ControlWindow::prev_seq( GtkWidget* widget,
                         class ControlWindow* cw_p )
{
  replay_arena.change_game( 0, -1 );
}

void
ControlWindow::change_current_replay_time( GtkAdjustment *adj,
                                           class ControlWindow* cw_p )
{
  replay_arena.change_replay_time( gtk_adjustment_get_value( adj ) );
}

void
ControlWindow::set_progress_time( const double time )
{
  gtk_adjustment_set_value( current_replay_time_adjustment, time );
}
