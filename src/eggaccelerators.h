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


#ifndef __EGG_ACCELERATORS_H__
#define __EGG_ACCELERATORS_H__

#include <gdk/gdk.h>

G_BEGIN_DECLS

/* Where a value is also in GdkModifierType we coincide,
 * otherwise we don't overlap.
 */
typedef enum
{
  EGG_VIRTUAL_SHIFT_MASK    = 1 << 0,
  EGG_VIRTUAL_LOCK_MASK	    = 1 << 1,
  EGG_VIRTUAL_CONTROL_MASK  = 1 << 2,

  EGG_VIRTUAL_ALT_MASK      = 1 << 3, /* fixed as Mod1 */

  EGG_VIRTUAL_MOD2_MASK	    = 1 << 4,
  EGG_VIRTUAL_MOD3_MASK	    = 1 << 5,
  EGG_VIRTUAL_MOD4_MASK	    = 1 << 6,
  EGG_VIRTUAL_MOD5_MASK	    = 1 << 7,

#if 0
  GDK_BUTTON1_MASK  = 1 << 8,
  GDK_BUTTON2_MASK  = 1 << 9,
  GDK_BUTTON3_MASK  = 1 << 10,
  GDK_BUTTON4_MASK  = 1 << 11,
  GDK_BUTTON5_MASK  = 1 << 12,
  /* 13, 14 are used by Xkb for the keyboard group */
#endif

  EGG_VIRTUAL_MODE_SWITCH_MASK = 1 << 23,
  EGG_VIRTUAL_NUM_LOCK_MASK = 1 << 24,
  EGG_VIRTUAL_SCROLL_LOCK_MASK = 1 << 25,

  /* Also in GdkModifierType */
  EGG_VIRTUAL_SUPER_MASK = 1 << 26,
  EGG_VIRTUAL_HYPER_MASK = 1 << 27,
  EGG_VIRTUAL_META_MASK = 1 << 28,

  /* Also in GdkModifierType */
  EGG_VIRTUAL_RELEASE_MASK  = 1 << 30,

  /*     28-31 24-27 20-23 16-19 12-15 8-11 4-7 0-3
   *       5     f     8     0     0    0    f   f
   */
  EGG_VIRTUAL_MODIFIER_MASK = 0x5f8000ff

} EggVirtualModifierType;

void     egg_keymap_resolve_virtual_modifiers (GdkKeymap              *keymap,
                                               EggVirtualModifierType  virtual_mods,
                                               GdkModifierType        *concrete_mods);

G_END_DECLS

#endif		/* __EGG_ACCELERATORS_H__ */

