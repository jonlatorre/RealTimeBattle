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

#include "Structs.h"
#include "ArenaController.h"
#include "ArenaRealTime.h"

pixmap_t::~pixmap_t()
{
  if( pixbuf != NULL )
    g_object_unref( pixbuf );
}

// Devuelve (creando y cacheando la primera vez) un GdkPixbuf 16x16 relleno
// con el color solido del robot.
GdkPixbuf*
pixmap_t::get_pixbuf(GdkColor& col)
{
  if( pixbuf == NULL )
    {
      pixbuf = gdk_pixbuf_new( GDK_COLORSPACE_RGB, FALSE, 8, 16, 16 );
      guint32 rgba =
        ( ( (guint32)( col.red   >> 8 ) ) << 24 ) |
        ( ( (guint32)( col.green >> 8 ) ) << 16 ) |
        ( ( (guint32)( col.blue  >> 8 ) ) <<  8 ) |
        0xff;
      gdk_pixbuf_fill( pixbuf, rgba );
    }
  return pixbuf;
}

#endif
