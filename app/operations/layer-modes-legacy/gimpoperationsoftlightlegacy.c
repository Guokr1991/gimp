/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoperationsoftlightmode.c
 * Copyright (C) 2008 Michael Natterer <mitch@gimp.org>
 *               2012 Ville Sokk <ville.sokk@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl-plugin.h>

#include "../operations-types.h"

#include "gimpoperationsoftlightlegacy.h"


static gboolean   gimp_operation_softlight_legacy_process (GeglOperation       *op,
                                                           void                *in,
                                                           void                *layer,
                                                           void                *mask,
                                                           void                *out,
                                                           glong                samples,
                                                           const GeglRectangle *roi,
                                                           gint                 level);


G_DEFINE_TYPE (GimpOperationSoftlightLegacy, gimp_operation_softlight_legacy,
               GIMP_TYPE_OPERATION_LAYER_MODE)


static const gchar* reference_xml = "<?xml version='1.0' encoding='UTF-8'?>"
"<gegl>"
"<node operation='gimp:softlight-legacy'>"
"  <node operation='gegl:load'>"
"    <params>"
"      <param name='path'>B.png</param>"
"    </params>"
"  </node>"
"</node>"
"<node operation='gegl:load'>"
"  <params>"
"    <param name='path'>A.png</param>"
"  </params>"
"</node>"
"</gegl>";


static void
gimp_operation_softlight_legacy_class_init (GimpOperationSoftlightLegacyClass *klass)
{
  GeglOperationClass          *operation_class  = GEGL_OPERATION_CLASS (klass);
  GimpOperationLayerModeClass *layer_mode_class = GIMP_OPERATION_LAYER_MODE_CLASS (klass);

  gegl_operation_class_set_keys (operation_class,
                                 "name",        "gimp:softlight-legacy",
                                 "description", "GIMP softlight mode operation",
                                 "reference-image", "soft-light-mode.png",
                                 "reference-composition", reference_xml,
                                 NULL);

  layer_mode_class->process = gimp_operation_softlight_legacy_process;
}

static void
gimp_operation_softlight_legacy_init (GimpOperationSoftlightLegacy *self)
{
}

static gboolean
gimp_operation_softlight_legacy_process (GeglOperation       *op,
                                         void                *in_p,
                                         void                *layer_p,
                                         void                *mask_p,
                                         void                *out_p,
                                         glong                samples,
                                         const GeglRectangle *roi,
                                         gint                 level)
{
  GimpOperationLayerMode *layer_mode = (gpointer) op;
  gfloat                 *in         = in_p;
  gfloat                 *out        = out_p;
  gfloat                 *layer      = layer_p;
  gfloat                 *mask       = mask_p;
  gfloat                  opacity    = layer_mode->opacity;

  while (samples--)
    {
      gfloat comp_alpha, new_alpha;

      comp_alpha = MIN (in[ALPHA], layer[ALPHA]) * opacity;
      if (mask)
        comp_alpha *= *mask;

      new_alpha = in[ALPHA] + (1.0 - in[ALPHA]) * comp_alpha;

      if (comp_alpha && new_alpha)
        {
          gfloat ratio = comp_alpha / new_alpha;
          gint   b;

          for (b = RED; b < ALPHA; b++)
            {
#if 0
              /* softlight is now used for what GIMP formerly called
               * OVERLAY.  We fixed OVERLAY to use the right math
               * (under the name NEW_OVERLAY), and redirect uses of
               * the old OVERLAY blend mode here. This math was
               * formerly used for OVERLAY and is exactly the same as
               * the multiply, screen, comp math used below.
               * See bug #673501.
               */
              gfloat comp = in[b] * (in[b] + (2.0 * layer[b]) * (1.0 - in[b]));
#endif

              gfloat multiply = in[b] * layer[b];
              gfloat screen   = 1.0 - (1.0 - in[b]) * (1.0 - layer[b]);
              gfloat comp     = (1.0 - in[b]) * multiply + in[b] * screen;

              out[b] = comp * ratio + in[b] * (1.0 - ratio);
            }
        }
      else
        {
          gint b;

          for (b = RED; b < ALPHA; b++)
            {
              out[b] = in[b];
            }
        }

      out[ALPHA] = in[ALPHA];

      in    += 4;
      layer += 4;
      out   += 4;

      if (mask)
        mask ++;
    }

  return TRUE;
}
