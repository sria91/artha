/* eggaccelerators.c
 * Artha - Free cross-platform open thesaurus
 * Copyright (C) 2009, 2010  Sundaram Ramaswamy, legends2k@yahoo.com
 *
 * Artha is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * Artha is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Artha; if not, write to the Free Software Foundation, Inc., 
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


/*
 * Ignorable modifiers in a hotkey combination like caps lock, 
 * num lock, scroll lock, etc. are looked up using this code - from 
 * the gnome-control-center project.
 *
 * Credits for this code goes fully to Havoc Pennington, hp@pobox.com
 * and Tim Janik, timj@gtk.org
 */


#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif

#ifdef X11_AVAILABLE

#include "eggaccelerators.h"

#include <stdlib.h>
#include <string.h>
#include <gdk/gdkx.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

enum
{
  EGG_MODMAP_ENTRY_SHIFT   = 0,
  EGG_MODMAP_ENTRY_LOCK    = 1,
  EGG_MODMAP_ENTRY_CONTROL = 2,
  EGG_MODMAP_ENTRY_MOD1    = 3,
  EGG_MODMAP_ENTRY_MOD2    = 4,
  EGG_MODMAP_ENTRY_MOD3    = 5,
  EGG_MODMAP_ENTRY_MOD4    = 6,
  EGG_MODMAP_ENTRY_MOD5    = 7,
  EGG_MODMAP_ENTRY_LAST    = 8
};

#define MODMAP_ENTRY_TO_MODIFIER(x) (1 << (x))

typedef struct
{
  EggVirtualModifierType mapping[EGG_MODMAP_ENTRY_LAST];

} EggModmap;

static void
reload_modmap (GdkKeymap *keymap,
               EggModmap *modmap)
{
  XModifierKeymap *xmodmap;
  int map_size;
  int i;

  /* FIXME multihead */
  xmodmap = XGetModifierMapping (gdk_x11_get_default_xdisplay ());

  memset (modmap->mapping, 0, sizeof (modmap->mapping));

  /* there are 8 modifiers in the order shift, shift lock,
   * control, mod1-5 with up to max_keypermod bindings each
   */
  map_size = 8 * xmodmap->max_keypermod;
  for (i = 3 * xmodmap->max_keypermod; i < map_size; ++i)
    {
      /* get the key code at this point in the map,
       * see if its keysym is one we're interested in
       */
      int keycode = xmodmap->modifiermap[i];
      GdkKeymapKey *keys;
      guint *keyvals;
      int n_entries;
      int j;
      EggVirtualModifierType mask;

      keys = NULL;
      keyvals = NULL;
      n_entries = 0;

      gdk_keymap_get_entries_for_keycode (keymap,
                                          keycode,
                                          &keys, &keyvals, &n_entries);

      mask = 0;
      for (j = 0; j < n_entries; ++j)
        {
          if (keyvals[j] == GDK_Num_Lock)
            mask |= EGG_VIRTUAL_NUM_LOCK_MASK;
          else if (keyvals[j] == GDK_Scroll_Lock)
            mask |= EGG_VIRTUAL_SCROLL_LOCK_MASK;
          else if (keyvals[j] == GDK_Meta_L ||
                   keyvals[j] == GDK_Meta_R)
            mask |= EGG_VIRTUAL_META_MASK;
          else if (keyvals[j] == GDK_Hyper_L ||
                   keyvals[j] == GDK_Hyper_R)
            mask |= EGG_VIRTUAL_HYPER_MASK;
          else if (keyvals[j] == GDK_Super_L ||
                   keyvals[j] == GDK_Super_R)
            mask |= EGG_VIRTUAL_SUPER_MASK;
          else if (keyvals[j] == GDK_Mode_switch)
            mask |= EGG_VIRTUAL_MODE_SWITCH_MASK;
        }

      /* Mod1Mask is 1 << 3 for example, i.e. the
       * fourth modifier, i / keyspermod is the modifier
       * index
       */
      modmap->mapping[i/xmodmap->max_keypermod] |= mask;

      g_free (keyvals);
      g_free (keys);
    }

  /* Add in the not-really-virtual fixed entries */
  modmap->mapping[EGG_MODMAP_ENTRY_SHIFT] |= EGG_VIRTUAL_SHIFT_MASK;
  modmap->mapping[EGG_MODMAP_ENTRY_CONTROL] |= EGG_VIRTUAL_CONTROL_MASK;
  modmap->mapping[EGG_MODMAP_ENTRY_LOCK] |= EGG_VIRTUAL_LOCK_MASK;
  modmap->mapping[EGG_MODMAP_ENTRY_MOD1] |= EGG_VIRTUAL_ALT_MASK;
  modmap->mapping[EGG_MODMAP_ENTRY_MOD2] |= EGG_VIRTUAL_MOD2_MASK;
  modmap->mapping[EGG_MODMAP_ENTRY_MOD3] |= EGG_VIRTUAL_MOD3_MASK;
  modmap->mapping[EGG_MODMAP_ENTRY_MOD4] |= EGG_VIRTUAL_MOD4_MASK;
  modmap->mapping[EGG_MODMAP_ENTRY_MOD5] |= EGG_VIRTUAL_MOD5_MASK;

  XFreeModifiermap (xmodmap);
}

static const EggModmap*
egg_keymap_get_modmap (GdkKeymap *keymap)
{
  EggModmap *modmap;

  if (keymap == NULL)
    keymap = gdk_keymap_get_default ();

  /* This is all a hack, much simpler when we can just
   * modify GDK directly.
   */

  modmap = g_object_get_data (G_OBJECT (keymap), "egg-modmap");

  if (modmap == NULL)
    {
      modmap = g_new0 (EggModmap, 1);

      /* FIXME modify keymap change events with an event filter
       * and force a reload if we get one
       */

      reload_modmap (keymap, modmap);

      g_object_set_data_full (G_OBJECT (keymap),
                              "egg-modmap",
                              modmap,
                              g_free);
    }

  g_assert (modmap != NULL);

  return modmap;
}

void
egg_keymap_resolve_virtual_modifiers (GdkKeymap              *keymap,
                                      EggVirtualModifierType  virtual_mods,
                                      GdkModifierType        *concrete_mods)
{
  GdkModifierType concrete;
  int i;
  const EggModmap *modmap;

  g_return_if_fail (concrete_mods != NULL);
  g_return_if_fail (keymap == NULL || GDK_IS_KEYMAP (keymap));

  modmap = egg_keymap_get_modmap (keymap);

  /* Not so sure about this algorithm. */

  concrete = 0;
  for (i = 0; i < EGG_MODMAP_ENTRY_LAST; ++i)
    {
      if (modmap->mapping[i] & virtual_mods)
        concrete |= MODMAP_ENTRY_TO_MODIFIER (i);
    }

  *concrete_mods = concrete;
}

#endif		/* X11_AVAILABLE */
