/*
*  Gtk Altimeter Widget
*  Copyright (C) 2010, CCNY Robotics Lab
*  Gautier Dumonteil <gautier.dumonteil@gmail.com>
*  http://robotics.ccny.cuny.edu
*
*  This program is free software: you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation, either version 3 of the License, or
*  (at your option) any later version.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <ground_station/gui/gtkaltimeter.h>

typedef struct _GtkAltimeterPrivate
{
  /* new cairo design */
  cairo_t *cr;
  GdkRectangle plot_box;

  /* widget data */
  gint unit_value;
  gboolean unit_is_feet;
  gboolean color_mode_inv;
  gboolean radial_color;
  gdouble altitude;

  /* drawing data */
  gdouble x;
  gdouble y;
  gdouble radius;
  GdkColor bg_color_inv;
  GdkColor bg_color_altimeter;
  GdkColor bg_color_bounderie;
  GdkColor bg_radial_color_begin_altimeter;
  GdkColor bg_radial_color_begin_bounderie;

  /* mouse information */
  gboolean b_mouse_onoff;
  GdkPoint mouse_pos;
  GdkModifierType mouse_state;

} GtkAltimeterPrivate;

enum _GLG_PROPERTY_ID
{
  PROP_0,
  PROP_INVERSED_COLOR,
  PROP_UNIT_IS_FEET,
  PROP_UNIT_STEP_VALUE,
  PROP_RADIAL_COLOR,
} GLG_PROPERTY_ID;

G_DEFINE_TYPE (GtkAltimeter, gtk_altimeter, GTK_TYPE_DRAWING_AREA);

#define GTK_ALTIMETER_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GTK_ALTIMETER_TYPE, GtkAltimeterPrivate))

static void gtk_altimeter_class_init (GtkAltimeterClass * klass);
static void gtk_altimeter_init (GtkAltimeter * alt);
static void gtk_altimeter_destroy (GtkObject * object);
static void gtk_altimeter_set_property (GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec);

static gboolean gtk_altimeter_configure_event (GtkWidget * widget, GdkEventConfigure * event);
static gboolean gtk_altimeter_expose (GtkWidget * graph, GdkEventExpose * event);
static gboolean gtk_altimeter_button_press_event (GtkWidget * widget, GdkEventButton * ev);
static gboolean gtk_altimeter_motion_notify_event (GtkWidget * widget, GdkEventMotion * ev);

static void gtk_altimeter_draw (GtkWidget * alt);
static void gtk_altimeter_draw_screws (GtkWidget * alt);
static void gtk_altimeter_draw_digital (GtkWidget * alt);
static void gtk_altimeter_draw_hands (GtkWidget * alt);

static gboolean gtk_altimeter_debug = FALSE;

static void gtk_altimeter_class_init (GtkAltimeterClass * klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GtkObjectClass *gtkobject_class = GTK_OBJECT_CLASS (klass);

  if (gtk_altimeter_debug)
  {
    g_debug ("===> gtk_altimeter_class_init()");
  }

  /* GObject signal overrides */
  obj_class->set_property = gtk_altimeter_set_property;

  /* GtkObject signal overrides */
  gtkobject_class->destroy = gtk_altimeter_destroy;

  /* GtkWidget signals overrides */
  widget_class->configure_event = gtk_altimeter_configure_event;
  widget_class->expose_event = gtk_altimeter_expose;
  widget_class->motion_notify_event = gtk_altimeter_motion_notify_event;
  widget_class->button_press_event = gtk_altimeter_button_press_event;

  g_type_class_add_private (obj_class, sizeof (GtkAltimeterPrivate));

  g_object_class_install_property (obj_class,
                                   PROP_INVERSED_COLOR,
                                   g_param_spec_boolean ("inverse-color",
                                                         "inverse or not the widget color",
                                                         "inverse or not the widget color", FALSE, G_PARAM_WRITABLE));
  g_object_class_install_property (obj_class,
                                   PROP_UNIT_IS_FEET,
                                   g_param_spec_boolean ("unit-is-feet",
                                                         "set the altimeter unit to feet or meter",
                                                         "set the altimeter unit to feet or meter",
                                                         TRUE, G_PARAM_WRITABLE));
  g_object_class_install_property (obj_class,
                                   PROP_UNIT_STEP_VALUE,
                                   g_param_spec_int ("unit-step-value",
                                                     "select the value of the initial step (1, 10 or 100)",
                                                     "select the value of the initial step (1, 10 or 100)",
                                                     1, 100, 100, G_PARAM_WRITABLE));
  g_object_class_install_property (obj_class,
                                   PROP_RADIAL_COLOR,
                                   g_param_spec_boolean ("radial-color",
                                                         "the widget use radial color",
                                                         "the widget use radial color", TRUE, G_PARAM_WRITABLE));
  return;
}

static void gtk_altimeter_init (GtkAltimeter * alt)
{
  GtkAltimeterPrivate *priv = NULL;

  if (gtk_altimeter_debug)
  {
    g_debug ("===> gtk_altimeter_init()");
  }
  g_return_if_fail (IS_GTK_ALTIMETER (alt));

  priv = GTK_ALTIMETER_GET_PRIVATE (alt);

  gtk_widget_add_events (GTK_WIDGET (alt), GDK_BUTTON_PRESS_MASK |
                         GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK);
  priv->b_mouse_onoff = FALSE;
  priv->color_mode_inv = FALSE;
  priv->radial_color = TRUE;

  priv->bg_color_bounderie.red = 6553.5;        // 0.1 cairo
  priv->bg_color_bounderie.green = 6553.5;
  priv->bg_color_bounderie.blue = 6553.5;
  priv->bg_color_altimeter.red = 3276.75;       // 0.05 cairo
  priv->bg_color_altimeter.green = 3276.75;
  priv->bg_color_altimeter.blue = 3276.75;
  priv->bg_color_inv.red = 45874.5;     // 0.7 cairo
  priv->bg_color_inv.green = 45874.5;
  priv->bg_color_inv.blue = 45874.5;
  priv->bg_radial_color_begin_bounderie.red = 13107;    // 0.2 cairo
  priv->bg_radial_color_begin_bounderie.green = 13107;
  priv->bg_radial_color_begin_bounderie.blue = 13107;
  priv->bg_radial_color_begin_altimeter.red = 45874.5;  // 0.7 cairo
  priv->bg_radial_color_begin_altimeter.green = 45874.5;
  priv->bg_radial_color_begin_altimeter.blue = 45874.5;
  return;
}

static gboolean gtk_altimeter_configure_event (GtkWidget * widget, GdkEventConfigure * event)
{
  GtkAltimeterPrivate *priv;
  GtkAltimeter *alt = GTK_ALTIMETER (widget);

  if (gtk_altimeter_debug)
  {
    g_debug ("===> gtk_altimeter_configure_event()");
  }
  g_return_val_if_fail (IS_GTK_ALTIMETER (alt), FALSE);

  g_return_val_if_fail (event->type == GDK_CONFIGURE, FALSE);

  priv = GTK_ALTIMETER_GET_PRIVATE (alt);
  g_return_val_if_fail (priv != NULL, FALSE);

  if (gtk_altimeter_debug)
  {
    g_debug ("===> gtk_altimeter_configure_event(new width=%d, height=%d)", event->width, event->height);
  }

  if ((event->width < GTK_ALTIMETER_MODEL_X) || (event->height < GTK_ALTIMETER_MODEL_Y))
  {
    priv->plot_box.width = GTK_ALTIMETER_MODEL_X;
    priv->plot_box.height = GTK_ALTIMETER_MODEL_Y;
  }
  else
  {
    priv->plot_box.width = event->width;
    priv->plot_box.height = event->height;
  }

  if (gtk_altimeter_debug)
  {
    g_debug ("cfg:Max.Avail: plot_box.width=%d, plot_box.height=%d", priv->plot_box.width, priv->plot_box.height);
  }
  return FALSE;
}

static gboolean gtk_altimeter_expose (GtkWidget * alt, GdkEventExpose * event)
{
  GtkAltimeterPrivate *priv;
  GtkWidget *widget = alt;

  cairo_t *cr = NULL;
  cairo_status_t status;

  if (gtk_altimeter_debug)
  {
    g_debug ("===> gtk_altimeter_expose()");
  }
  g_return_val_if_fail (IS_GTK_ALTIMETER (alt), FALSE);

  priv = GTK_ALTIMETER_GET_PRIVATE (alt);
  g_return_val_if_fail (priv != NULL, FALSE);

  priv->plot_box.width = widget->allocation.width;
  priv->plot_box.height = widget->allocation.height;

  if (gtk_altimeter_debug)
  {
    g_debug ("gtk_altimeter_expose(width=%d, height=%d)", widget->allocation.width, widget->allocation.height);
  }

  priv->cr = cr = gdk_cairo_create (widget->window);
  status = cairo_status (cr);
  if (status != CAIRO_STATUS_SUCCESS)
  {
    g_message ("GLG-Expose:cairo_create:status %d=%s", status, cairo_status_to_string (status));
  }

  cairo_rectangle (cr, 0, 0, priv->plot_box.width, priv->plot_box.height);
  cairo_clip (cr);

  gtk_altimeter_draw (alt);

  cairo_destroy (cr);
  priv->cr = NULL;

  return FALSE;
}

extern void gtk_altimeter_redraw (GtkAltimeter * alt)
{
  GtkWidget *widget;
  GdkRegion *region;

  if (gtk_altimeter_debug)
  {
    g_debug ("===> gtk_altimeter_redraw()");
  }
  g_return_if_fail (IS_GTK_ALTIMETER (alt));

  widget = GTK_WIDGET (alt);

  if (!widget->window)
    return;

  region = gdk_drawable_get_clip_region (widget->window);
  /* redraw the window completely by exposing it */
  gdk_window_invalidate_region (widget->window, region, TRUE);
  gdk_window_process_updates (widget->window, TRUE);

  gdk_region_destroy (region);
}

extern void gtk_altimeter_set_alti (GtkAltimeter * alt, gdouble alti)
{
  GtkAltimeterPrivate *priv;

  if (gtk_altimeter_debug)
  {
    g_debug ("===> gtk_altimeter_draw()");
  }
  g_return_if_fail (IS_GTK_ALTIMETER (alt));

  priv = GTK_ALTIMETER_GET_PRIVATE (alt);
  priv->altitude = alti;
}

extern GtkWidget *gtk_altimeter_new (void)
{
  if (gtk_altimeter_debug)
  {
    g_debug ("===> gtk_altimeter_new()");
  }
  return GTK_WIDGET (gtk_type_new (gtk_altimeter_get_type ()));
}

static void gtk_altimeter_draw (GtkWidget * alt)
{
  GtkAltimeterPrivate *priv;

  if (gtk_altimeter_debug)
  {
    g_debug ("===> gtk_altimeter_draw()");
  }
  g_return_if_fail (IS_GTK_ALTIMETER (alt));

  priv = GTK_ALTIMETER_GET_PRIVATE (alt);

  double x, y, rec_x0, rec_y0, rec_width, rec_height, rec_degrees;
  double rec_aspect, rec_corner_radius, rec_radius, radius;
  char str[GTK_ALTIMETER_MAX_STRING];
  int i, factor;

  x = alt->allocation.width / 2;
  y = alt->allocation.height / 2;
  radius = MIN (alt->allocation.width / 2, alt->allocation.height / 2) - 5;
  cairo_pattern_t *pat;

  rec_x0 = x - radius;
  rec_y0 = y - radius;
  rec_width = radius * 2;
  rec_height = radius * 2;
  rec_aspect = 1.0;
  rec_corner_radius = rec_height / 8.0;

  rec_radius = rec_corner_radius / rec_aspect;
  rec_degrees = M_PI / 180.0;

  // Altimeter base
  cairo_new_sub_path (priv->cr);
  cairo_arc (priv->cr, rec_x0 + rec_width - rec_radius, rec_y0 + rec_radius,
             rec_radius, -90 * rec_degrees, 0 * rec_degrees);
  cairo_arc (priv->cr, rec_x0 + rec_width - rec_radius, rec_y0 + rec_height - rec_radius,
             rec_radius, 0 * rec_degrees, 90 * rec_degrees);
  cairo_arc (priv->cr, rec_x0 + rec_radius, rec_y0 + rec_height - rec_radius,
             rec_radius, 90 * rec_degrees, 180 * rec_degrees);
  cairo_arc (priv->cr, rec_x0 + rec_radius, rec_y0 + rec_radius, rec_radius, 180 * rec_degrees, 270 * rec_degrees);
  cairo_close_path (priv->cr);

  if (((priv->radial_color) && (priv->color_mode_inv)) || ((!priv->radial_color) && (priv->color_mode_inv))
      || ((!priv->radial_color) && (!priv->color_mode_inv)))
  {
    if (!priv->color_mode_inv)
      cairo_set_source_rgb (priv->cr, (gdouble) priv->bg_color_bounderie.red / 65535,
                            (gdouble) priv->bg_color_bounderie.green / 65535,
                            (gdouble) priv->bg_color_bounderie.blue / 65535);
    else
      cairo_set_source_rgb (priv->cr, (gdouble) priv->bg_color_inv.red / 65535,
                            (gdouble) priv->bg_color_inv.green / 65535, (gdouble) priv->bg_color_inv.blue / 65535);
  }
  else
  {
    pat = cairo_pattern_create_radial (x - 0.392 * radius, y - 0.967 * radius, 0.167 * radius,
                                       x - 0.477 * radius, y - 0.967 * radius, 0.836 * radius);
    cairo_pattern_add_color_stop_rgba (pat, 0, (gdouble) priv->bg_radial_color_begin_bounderie.red / 65535,
                                       (gdouble) priv->bg_radial_color_begin_bounderie.green / 65535,
                                       (gdouble) priv->bg_radial_color_begin_bounderie.blue / 65535, 1);
    cairo_pattern_add_color_stop_rgba (pat, 1, (gdouble) priv->bg_color_bounderie.red / 65535,
                                       (gdouble) priv->bg_color_bounderie.green / 65535,
                                       (gdouble) priv->bg_color_bounderie.blue / 65535, 1);
    cairo_set_source (priv->cr, pat);
  }
  cairo_fill_preserve (priv->cr);
  cairo_stroke (priv->cr);

  cairo_arc (priv->cr, x, y, radius, 0, 2 * M_PI);
  if (!priv->color_mode_inv)
    cairo_set_source_rgb (priv->cr, 0., 0., 0.);
  else
    cairo_set_source_rgb (priv->cr, 1., 1., 1.);
  cairo_fill_preserve (priv->cr);
  cairo_stroke (priv->cr);

  cairo_arc (priv->cr, x, y, radius - 0.04 * radius, 0, 2 * M_PI);
  if (!priv->color_mode_inv)
    cairo_set_source_rgb (priv->cr, 0.6, 0.5, 0.5);
  else
    cairo_set_source_rgb (priv->cr, 1 - 0.6, 1 - 0.5, 1 - 0.5);
  cairo_stroke (priv->cr);

  cairo_set_line_width (priv->cr, 0.01 * radius);
  radius = radius - 0.1 * radius;
  cairo_arc (priv->cr, x, y, radius, 0, 2 * M_PI);
  if (((priv->radial_color) && (priv->color_mode_inv)) || ((!priv->radial_color) && (priv->color_mode_inv))
      || ((!priv->radial_color) && (!priv->color_mode_inv)))
  {
    if (!priv->color_mode_inv)
      cairo_set_source_rgb (priv->cr, (gdouble) priv->bg_color_altimeter.red / 65535,
                            (gdouble) priv->bg_color_altimeter.green / 65535,
                            (gdouble) priv->bg_color_altimeter.blue / 65535);
    else
      cairo_set_source_rgb (priv->cr, (gdouble) priv->bg_color_inv.red / 65535,
                            (gdouble) priv->bg_color_inv.green / 65535, (gdouble) priv->bg_color_inv.blue / 65535);
  }
  else
  {
    pat = cairo_pattern_create_radial (x - 0.392 * radius, y - 0.967 * radius, 0.167 * radius,
                                       x - 0.477 * radius, y - 0.967 * radius, 0.836 * radius);
    cairo_pattern_add_color_stop_rgba (pat, 0, (gdouble) priv->bg_radial_color_begin_altimeter.red / 65535,
                                       (gdouble) priv->bg_radial_color_begin_altimeter.green / 65535,
                                       (gdouble) priv->bg_radial_color_begin_altimeter.blue / 65535, 1);
    cairo_pattern_add_color_stop_rgba (pat, 1, (gdouble) priv->bg_color_altimeter.red / 65535,
                                       (gdouble) priv->bg_color_altimeter.green / 65535,
                                       (gdouble) priv->bg_color_altimeter.blue / 65535, 1);
    cairo_set_source (priv->cr, pat);
  }
  cairo_fill_preserve (priv->cr);
  cairo_stroke (priv->cr);

  cairo_arc (priv->cr, x, y, radius, 0, 2 * M_PI);
  if (!priv->color_mode_inv)
    cairo_set_source_rgb (priv->cr, 0.6, 0.6, 0.6);
  else
    cairo_set_source_rgb (priv->cr, 1 - 0.6, 1 - 0.6, 1 - 0.6);
  cairo_fill_preserve (priv->cr);
  cairo_stroke (priv->cr);

  cairo_arc (priv->cr, x, y, radius - 0.07 * radius, 0, 2 * M_PI);
  if (((priv->radial_color) && (priv->color_mode_inv)) || ((!priv->radial_color) && (priv->color_mode_inv))
      || ((!priv->radial_color) && (!priv->color_mode_inv)))
  {
    if (!priv->color_mode_inv)
      cairo_set_source_rgb (priv->cr, (gdouble) priv->bg_color_altimeter.red / 65535,
                            (gdouble) priv->bg_color_altimeter.green / 65535,
                            (gdouble) priv->bg_color_altimeter.blue / 65535);
    else
      cairo_set_source_rgb (priv->cr, (gdouble) priv->bg_color_inv.red / 65535,
                            (gdouble) priv->bg_color_inv.green / 65535, (gdouble) priv->bg_color_inv.blue / 65535);
  }
  else
  {
    pat = cairo_pattern_create_radial (x - 0.392 * radius, y - 0.967 * radius, 0.167 * radius,
                                       x - 0.477 * radius, y - 0.967 * radius, 0.836 * radius);
    cairo_pattern_add_color_stop_rgba (pat, 0, (gdouble) priv->bg_radial_color_begin_altimeter.red / 65535,
                                       (gdouble) priv->bg_radial_color_begin_altimeter.green / 65535,
                                       (gdouble) priv->bg_radial_color_begin_altimeter.blue / 65535, 1);
    cairo_pattern_add_color_stop_rgba (pat, 1, (gdouble) priv->bg_color_altimeter.red / 65535,
                                       (gdouble) priv->bg_color_altimeter.green / 65535,
                                       (gdouble) priv->bg_color_altimeter.blue / 65535, 1);
    cairo_set_source (priv->cr, pat);
  }
  cairo_fill (priv->cr);
  cairo_stroke (priv->cr);

  // fun
  cairo_arc (priv->cr, x, y, radius - 0.45 * radius, M_PI / 3, 2 * M_PI / 3);
  cairo_move_to (priv->cr, x + 0.27 * radius, y + 0.4764 * radius);
  cairo_arc (priv->cr, x, y, radius - 0.8 * radius, M_PI / 3, 2 * M_PI / 3);
  cairo_line_to (priv->cr, x - 0.27 * radius, y + 0.4764 * radius);
  if (!priv->color_mode_inv)
    cairo_set_source_rgb (priv->cr, 0.9, 0.9, 0.9);
  else
    cairo_set_source_rgb (priv->cr, 1 - 0.9, 1 - 0.9, 1 - 0.9);
  cairo_fill_preserve (priv->cr);
  cairo_stroke (priv->cr);

  cairo_set_line_width (priv->cr, 0.03 * radius);
  cairo_move_to (priv->cr, x - 0.7 * radius, y);
  cairo_line_to (priv->cr, x - 0.1 * radius, y + 0.6 * radius);
  cairo_move_to (priv->cr, x - 0.6 * radius, y);
  cairo_line_to (priv->cr, x + 0 * radius, y + 0.6 * radius);
  cairo_move_to (priv->cr, x - 0.5 * radius, y);
  cairo_line_to (priv->cr, x + 0.1 * radius, y + 0.6 * radius);
  cairo_move_to (priv->cr, x - 0.4 * radius, y);
  cairo_line_to (priv->cr, x + 0.2 * radius, y + 0.6 * radius);
  cairo_move_to (priv->cr, x - 0.3 * radius, y);
  cairo_line_to (priv->cr, x + 0.3 * radius, y + 0.6 * radius);
  cairo_move_to (priv->cr, x - 0.2 * radius, y);
  cairo_line_to (priv->cr, x + 0.4 * radius, y + 0.6 * radius);
  cairo_move_to (priv->cr, x - 0.1 * radius, y);
  cairo_line_to (priv->cr, x + 0.5 * radius, y + 0.6 * radius);

  if (((priv->radial_color) && (priv->color_mode_inv)) || ((!priv->radial_color) && (priv->color_mode_inv))
      || ((!priv->radial_color) && (!priv->color_mode_inv)))
  {
    if (!priv->color_mode_inv)
      cairo_set_source_rgb (priv->cr, (gdouble) priv->bg_color_altimeter.red / 65535,
                            (gdouble) priv->bg_color_altimeter.green / 65535,
                            (gdouble) priv->bg_color_altimeter.blue / 65535);
    else
      cairo_set_source_rgb (priv->cr, (gdouble) priv->bg_color_inv.red / 65535,
                            (gdouble) priv->bg_color_inv.green / 65535, (gdouble) priv->bg_color_inv.blue / 65535);
  }
  else
  {
    pat = cairo_pattern_create_radial (x - 0.392 * radius, y - 0.967 * radius, 0.167 * radius,
                                       x - 0.477 * radius, y - 0.967 * radius, 0.836 * radius);
    cairo_pattern_add_color_stop_rgba (pat, 0, (gdouble) priv->bg_radial_color_begin_altimeter.red / 65535,
                                       (gdouble) priv->bg_radial_color_begin_altimeter.green / 65535,
                                       (gdouble) priv->bg_radial_color_begin_altimeter.blue / 65535, 1);
    cairo_pattern_add_color_stop_rgba (pat, 1, (gdouble) priv->bg_color_altimeter.red / 65535,
                                       (gdouble) priv->bg_color_altimeter.green / 65535,
                                       (gdouble) priv->bg_color_altimeter.blue / 65535, 1);
    cairo_set_source (priv->cr, pat);
  }
  cairo_stroke (priv->cr);

  // Altimeter ticks 
  if (!priv->color_mode_inv)
    cairo_set_source_rgb (priv->cr, 1, 1, 1);
  else
    cairo_set_source_rgb (priv->cr, 0., 0., 0.);
  for (i = 0; i < 50; i++)
  {
    int inset;
    cairo_save (priv->cr);

    if (i % 5 == 0)
      inset = 0.12 * radius;
    else
    {
      inset = 0.06 * radius;
      cairo_set_line_width (priv->cr, 0.5 * cairo_get_line_width (priv->cr));
    }

    cairo_move_to (priv->cr, x + (radius - inset) * cos (M_PI / 2 + i * M_PI / 25),
                   y + (radius - inset) * sin (M_PI / 2 + i * M_PI / 25));
    cairo_line_to (priv->cr, x + radius * cos (M_PI / 2 + i * M_PI / 25), y + radius * sin (M_PI / 2 + i * M_PI / 25));
    cairo_stroke (priv->cr);
    cairo_restore (priv->cr);
  }

  // "Altimeter" drawing
  cairo_select_font_face (priv->cr, "Sans", CAIRO_FONT_SLANT_OBLIQUE, CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_size (priv->cr, 0.1 * radius);
  cairo_move_to (priv->cr, x - 0.23 * radius, y - 0.12 * radius);
  cairo_show_text (priv->cr, "Altimeter");
  cairo_stroke (priv->cr);


  cairo_select_font_face (priv->cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
  if (priv->unit_is_feet)
  {
    // drawing unit : FEET
    cairo_save (priv->cr);
    cairo_set_font_size (priv->cr, 0.07 * radius);
    cairo_move_to (priv->cr, x + 0.18 * radius, y - 0.83 * radius);
    cairo_rotate (priv->cr, M_PI / 10);
    cairo_show_text (priv->cr, "FEET");
    cairo_stroke (priv->cr);
    cairo_restore (priv->cr);
  }
  else
  {
    // drawing unit : METER
    cairo_save (priv->cr);
    cairo_set_font_size (priv->cr, 0.07 * radius);
    cairo_move_to (priv->cr, x + 0.145 * radius, y - 0.85 * radius);
    cairo_rotate (priv->cr, M_PI / 10);
    cairo_show_text (priv->cr, "METER");
    cairo_stroke (priv->cr);
    cairo_restore (priv->cr);
  }
  switch (priv->unit_value)
  {
    case 1:
      cairo_save (priv->cr);
      cairo_set_font_size (priv->cr, 0.07 * radius);
      cairo_move_to (priv->cr, x - 0.29 * radius, y - 0.81 * radius);
      cairo_rotate (priv->cr, -M_PI / 10);
      sprintf (str, "%d", priv->unit_value);
      cairo_show_text (priv->cr, str);
      cairo_stroke (priv->cr);
      cairo_restore (priv->cr);
      factor = 100;
      break;
    case 10:
      cairo_save (priv->cr);
      cairo_set_font_size (priv->cr, 0.07 * radius);
      cairo_move_to (priv->cr, x - 0.31 * radius, y - 0.8 * radius);
      cairo_rotate (priv->cr, -M_PI / 10);
      sprintf (str, "%d", priv->unit_value);
      cairo_show_text (priv->cr, str);
      cairo_stroke (priv->cr);
      cairo_restore (priv->cr);
      factor = 10;
      break;
    case 100:
      cairo_save (priv->cr);
      cairo_set_font_size (priv->cr, 0.07 * radius);
      cairo_move_to (priv->cr, x - 0.33 * radius, y - 0.78 * radius);
      cairo_rotate (priv->cr, -M_PI / 10);
      sprintf (str, "%d", priv->unit_value);
      cairo_show_text (priv->cr, str);
      cairo_stroke (priv->cr);
      cairo_restore (priv->cr);
      factor = 1;
      break;
  }

  // Number drawing
  for (i = 0; i < 10; i++)
  {
    int inset;
    cairo_select_font_face (priv->cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size (priv->cr, 0.20 * radius);
    inset = 0.225 * radius;
    cairo_move_to (priv->cr, x - 0.065 * radius + (radius - inset) * cos (M_PI / 2 + i * M_PI / 5 + M_PI),
                   y + 0.07 * radius + (radius - inset) * sin (M_PI / 2 + i * M_PI / 5 + M_PI));
    sprintf (str, "%d", i);
    cairo_show_text (priv->cr, str);
    cairo_stroke (priv->cr);
  }

  priv->radius = radius;
  priv->x = x;
  priv->y = y;
  gtk_altimeter_draw_screws(alt);

  // draw digital 
  gtk_altimeter_draw_digital (alt);

  // draw hands
  gtk_altimeter_draw_hands (alt);
  return;
}


static void gtk_altimeter_draw_screws (GtkWidget * alt)
{
  GtkAltimeterPrivate *priv;

  if (gtk_altimeter_debug)
  {
    g_debug ("===> gtk_altimeter_draw()");
  }
  g_return_if_fail (IS_GTK_ALTIMETER (alt));

  priv = GTK_ALTIMETER_GET_PRIVATE (alt);
  
  cairo_pattern_t *pat=NULL;
  double x, y, radius;
  radius=priv->radius;
  x=priv->x;
  y=priv->y;
  radius = radius+0.12*radius;
  
  // **** top left screw
  cairo_arc (priv->cr, x-0.82*radius, y-0.82*radius, 0.1 * radius, 0, 2*M_PI);
  pat = cairo_pattern_create_radial (x-0.82*radius, y-0.82*radius, 0.07*radius,
                                     x-0.82*radius, y-0.82*radius, 0.1*radius);
  cairo_pattern_add_color_stop_rgba (pat,0, 0, 0, 0,0.7);
  cairo_pattern_add_color_stop_rgba (pat,1, 0, 0, 0,0.1);
  cairo_set_source (priv->cr, pat);
  cairo_fill_preserve (priv->cr);
  cairo_stroke (priv->cr);
    
  cairo_arc (priv->cr, x-0.82*radius, y-0.82*radius, 0.07 * radius, 0, 2*M_PI);
  if (((priv->radial_color) && (priv->color_mode_inv)) || ((!priv->radial_color) && (priv->color_mode_inv))
      || ((!priv->radial_color) && (!priv->color_mode_inv)))
  {
    if (!priv->color_mode_inv)
      cairo_set_source_rgb (priv->cr, (gdouble) priv->bg_color_bounderie.red / 65535,
                            (gdouble) priv->bg_color_bounderie.green / 65535,
                            (gdouble) priv->bg_color_bounderie.blue / 65535);
    else
      cairo_set_source_rgb (priv->cr, (gdouble) priv->bg_color_inv.red / 65535,
                            (gdouble) priv->bg_color_inv.green / 65535, (gdouble) priv->bg_color_inv.blue / 65535);
  }
  else
  {
    pat = cairo_pattern_create_radial (x - 0.392 * radius, y - 0.967 * radius, 0.167 * radius,
                                       x - 0.477 * radius, y - 0.967 * radius, 0.836 * radius);
    cairo_pattern_add_color_stop_rgba (pat, 0, (gdouble) priv->bg_radial_color_begin_bounderie.red / 65535,
                                       (gdouble) priv->bg_radial_color_begin_bounderie.green / 65535,
                                       (gdouble) priv->bg_radial_color_begin_bounderie.blue / 65535, 1);
    cairo_pattern_add_color_stop_rgba (pat, 1, 0.15,0.15,0.15, 1);
    cairo_set_source (priv->cr, pat);
  }
  cairo_fill_preserve (priv->cr);
  cairo_stroke (priv->cr);
  
  cairo_set_line_width (priv->cr, 0.02 * radius);
  if (!priv->color_mode_inv)
       cairo_set_source_rgb (priv->cr, 0., 0., 0.);
  else
       cairo_set_source_rgb (priv->cr, 1., 1., 1.);
  cairo_move_to (priv->cr, x-0.88*radius, y-0.82*radius);
  cairo_line_to (priv->cr, x-0.76*radius, y-0.82*radius);
  cairo_fill_preserve (priv->cr);
  cairo_stroke (priv->cr);  
  cairo_move_to (priv->cr, x-0.82*radius, y-0.88*radius);
  cairo_line_to (priv->cr, x-0.82*radius, y-0.76*radius);
  cairo_fill_preserve (priv->cr);
  cairo_stroke (priv->cr);
  cairo_set_line_width (priv->cr, 0.01 * radius);
  if (!priv->color_mode_inv)
		cairo_set_source_rgb (priv->cr, 0.1, 0.1, 0.1);
  else
      cairo_set_source_rgb (priv->cr, 0.9, 0.9, 0.9);  
  cairo_move_to (priv->cr, x-0.88*radius, y-0.82*radius);
  cairo_line_to (priv->cr, x-0.76*radius, y-0.82*radius);
  cairo_fill_preserve (priv->cr);
  cairo_stroke (priv->cr);  
  cairo_move_to (priv->cr, x-0.82*radius, y-0.88*radius);
  cairo_line_to (priv->cr, x-0.82*radius, y-0.76*radius);
  cairo_fill_preserve (priv->cr);
  cairo_stroke (priv->cr);      
  
   // **** top right screw
  cairo_arc (priv->cr, x+0.82*radius, y-0.82*radius, 0.1 * radius, 0, 2*M_PI);
  pat = cairo_pattern_create_radial (x+0.82*radius, y-0.82*radius, 0.07*radius,
                                     x+0.82*radius, y-0.82*radius, 0.1*radius);
  cairo_pattern_add_color_stop_rgba (pat,0, 0, 0, 0,0.7);
  cairo_pattern_add_color_stop_rgba (pat,1, 0, 0, 0,0.1);
  cairo_set_source (priv->cr, pat);
  cairo_fill_preserve (priv->cr);
  cairo_stroke (priv->cr);
    
  cairo_arc (priv->cr, x+0.82*radius, y-0.82*radius, 0.07 * radius, 0, 2*M_PI);
  if (((priv->radial_color) && (priv->color_mode_inv)) || ((!priv->radial_color) && (priv->color_mode_inv))
      || ((!priv->radial_color) && (!priv->color_mode_inv)))
  {
    if (!priv->color_mode_inv)
      cairo_set_source_rgb (priv->cr, (gdouble) priv->bg_color_bounderie.red / 65535,
                            (gdouble) priv->bg_color_bounderie.green / 65535,
                            (gdouble) priv->bg_color_bounderie.blue / 65535);
    else
      cairo_set_source_rgb (priv->cr, (gdouble) priv->bg_color_inv.red / 65535,
                            (gdouble) priv->bg_color_inv.green / 65535, (gdouble) priv->bg_color_inv.blue / 65535);
  }
  else
  {
    pat = cairo_pattern_create_radial (x - 0.392 * radius, y - 0.967 * radius, 0.167 * radius,
                                       x - 0.477 * radius, y - 0.967 * radius, 0.836 * radius);
    cairo_pattern_add_color_stop_rgba (pat, 0, (gdouble) priv->bg_radial_color_begin_bounderie.red / 65535,
                                       (gdouble) priv->bg_radial_color_begin_bounderie.green / 65535,
                                       (gdouble) priv->bg_radial_color_begin_bounderie.blue / 65535, 1);
    cairo_pattern_add_color_stop_rgba (pat, 1, 0.15,0.15,0.15, 1);
    cairo_set_source (priv->cr, pat);
  }
  cairo_fill_preserve (priv->cr);
  cairo_stroke (priv->cr);
  
  cairo_set_line_width (priv->cr, 0.02 * radius);
  if (!priv->color_mode_inv)
       cairo_set_source_rgb (priv->cr, 0., 0., 0.);
  else
       cairo_set_source_rgb (priv->cr, 1., 1., 1.);
  cairo_move_to (priv->cr, x+0.88*radius, y-0.82*radius);
  cairo_line_to (priv->cr, x+0.76*radius, y-0.82*radius);
  cairo_fill_preserve (priv->cr);
  cairo_stroke (priv->cr);  
  cairo_move_to (priv->cr, x+0.82*radius, y-0.88*radius);
  cairo_line_to (priv->cr, x+0.82*radius, y-0.76*radius);
  cairo_fill_preserve (priv->cr);
  cairo_stroke (priv->cr);
  cairo_set_line_width (priv->cr, 0.01 * radius);
  if (!priv->color_mode_inv)
		cairo_set_source_rgb (priv->cr, 0.1, 0.1, 0.1);
  else
      cairo_set_source_rgb (priv->cr, 0.9, 0.9, 0.9);  
  cairo_move_to (priv->cr, x+0.88*radius, y-0.82*radius);
  cairo_line_to (priv->cr, x+0.76*radius, y-0.82*radius);
  cairo_fill_preserve (priv->cr);
  cairo_stroke (priv->cr);  
  cairo_move_to (priv->cr, x+0.82*radius, y-0.88*radius);
  cairo_line_to (priv->cr, x+0.82*radius, y-0.76*radius);
  cairo_fill_preserve (priv->cr);
  cairo_stroke (priv->cr);     
  
   // **** bottom left screw
  cairo_arc (priv->cr, x-0.82*radius, y+0.82*radius, 0.1 * radius, 0, 2*M_PI);
  pat = cairo_pattern_create_radial (x-0.82*radius, y+0.82*radius, 0.07*radius,
                                     x-0.82*radius, y+0.82*radius, 0.1*radius);
  cairo_pattern_add_color_stop_rgba (pat,0, 0, 0, 0,0.7);
  cairo_pattern_add_color_stop_rgba (pat,1, 0, 0, 0,0.1);
  cairo_set_source (priv->cr, pat);
  cairo_fill_preserve (priv->cr);
  cairo_stroke (priv->cr);
    
  cairo_arc (priv->cr, x-0.82*radius, y+0.82*radius, 0.07 * radius, 0, 2*M_PI);
  if (((priv->radial_color) && (priv->color_mode_inv)) || ((!priv->radial_color) && (priv->color_mode_inv))
      || ((!priv->radial_color) && (!priv->color_mode_inv)))
  {
    if (!priv->color_mode_inv)
      cairo_set_source_rgb (priv->cr, (gdouble) priv->bg_color_bounderie.red / 65535,
                            (gdouble) priv->bg_color_bounderie.green / 65535,
                            (gdouble) priv->bg_color_bounderie.blue / 65535);
    else
      cairo_set_source_rgb (priv->cr, (gdouble) priv->bg_color_inv.red / 65535,
                            (gdouble) priv->bg_color_inv.green / 65535, (gdouble) priv->bg_color_inv.blue / 65535);
  }
  else
  {
    pat = cairo_pattern_create_radial (x - 0.392 * radius, y - 0.967 * radius, 0.167 * radius,
                                       x - 0.477 * radius, y - 0.967 * radius, 0.836 * radius);
    cairo_pattern_add_color_stop_rgba (pat, 0, (gdouble) priv->bg_radial_color_begin_bounderie.red / 65535,
                                       (gdouble) priv->bg_radial_color_begin_bounderie.green / 65535,
                                       (gdouble) priv->bg_radial_color_begin_bounderie.blue / 65535, 1);
    cairo_pattern_add_color_stop_rgba (pat, 1, 0.15,0.15,0.15, 1);
    cairo_set_source (priv->cr, pat);
  }
  cairo_fill_preserve (priv->cr);
  cairo_stroke (priv->cr);
  
  cairo_set_line_width (priv->cr, 0.02 * radius);
  if (!priv->color_mode_inv)
       cairo_set_source_rgb (priv->cr, 0., 0., 0.);
  else
       cairo_set_source_rgb (priv->cr, 1., 1., 1.);
  cairo_move_to (priv->cr, x-0.88*radius, y+0.82*radius);
  cairo_line_to (priv->cr, x-0.76*radius, y+0.82*radius);
  cairo_fill_preserve (priv->cr);
  cairo_stroke (priv->cr);  
  cairo_move_to (priv->cr, x-0.82*radius, y+0.88*radius);
  cairo_line_to (priv->cr, x-0.82*radius, y+0.76*radius);
  cairo_fill_preserve (priv->cr);
  cairo_stroke (priv->cr);
  cairo_set_line_width (priv->cr, 0.01 * radius);
  if (!priv->color_mode_inv)
		cairo_set_source_rgb (priv->cr, 0.1, 0.1, 0.1);
  else
      cairo_set_source_rgb (priv->cr, 0.9, 0.9, 0.9);  
  cairo_move_to (priv->cr, x-0.88*radius, y+0.82*radius);
  cairo_line_to (priv->cr, x-0.76*radius, y+0.82*radius);
  cairo_fill_preserve (priv->cr);
  cairo_stroke (priv->cr);  
  cairo_move_to (priv->cr, x-0.82*radius, y+0.88*radius);
  cairo_line_to (priv->cr, x-0.82*radius, y+0.76*radius);
  cairo_fill_preserve (priv->cr);
  cairo_stroke (priv->cr);     
  
   // **** bottom right screw
  cairo_arc (priv->cr, x+0.82*radius, y+0.82*radius, 0.1 * radius, 0, 2*M_PI);
  pat = cairo_pattern_create_radial (x+0.82*radius, y+0.82*radius, 0.07*radius,
                                     x+0.82*radius, y+0.82*radius, 0.1*radius);
  cairo_pattern_add_color_stop_rgba (pat,0, 0, 0, 0,0.7);
  cairo_pattern_add_color_stop_rgba (pat,1, 0, 0, 0,0.1);
  cairo_set_source (priv->cr, pat);
  cairo_fill_preserve (priv->cr);
  cairo_stroke (priv->cr);
    
  cairo_arc (priv->cr, x+0.82*radius, y+0.82*radius, 0.07 * radius, 0, 2*M_PI);
  if (((priv->radial_color) && (priv->color_mode_inv)) || ((!priv->radial_color) && (priv->color_mode_inv))
      || ((!priv->radial_color) && (!priv->color_mode_inv)))
  {
    if (!priv->color_mode_inv)
      cairo_set_source_rgb (priv->cr, (gdouble) priv->bg_color_bounderie.red / 65535,
                            (gdouble) priv->bg_color_bounderie.green / 65535,
                            (gdouble) priv->bg_color_bounderie.blue / 65535);
    else
      cairo_set_source_rgb (priv->cr, (gdouble) priv->bg_color_inv.red / 65535,
                            (gdouble) priv->bg_color_inv.green / 65535, (gdouble) priv->bg_color_inv.blue / 65535);
  }
  else
  {
    pat = cairo_pattern_create_radial (x - 0.392 * radius, y - 0.967 * radius, 0.167 * radius,
                                       x - 0.477 * radius, y - 0.967 * radius, 0.836 * radius);
    cairo_pattern_add_color_stop_rgba (pat, 0, (gdouble) priv->bg_radial_color_begin_bounderie.red / 65535,
                                       (gdouble) priv->bg_radial_color_begin_bounderie.green / 65535,
                                       (gdouble) priv->bg_radial_color_begin_bounderie.blue / 65535, 1);
    cairo_pattern_add_color_stop_rgba (pat, 1, 0.15,0.15,0.15, 1);
    cairo_set_source (priv->cr, pat);
  }
  cairo_fill_preserve (priv->cr);
  cairo_stroke (priv->cr);
  
  cairo_set_line_width (priv->cr, 0.02 * radius);
  if (!priv->color_mode_inv)
       cairo_set_source_rgb (priv->cr, 0., 0., 0.);
  else
       cairo_set_source_rgb (priv->cr, 1., 1., 1.);
  cairo_move_to (priv->cr, x+0.88*radius, y+0.82*radius);
  cairo_line_to (priv->cr, x+0.76*radius, y+0.82*radius);
  cairo_fill_preserve (priv->cr);
  cairo_stroke (priv->cr);  
  cairo_move_to (priv->cr, x+0.82*radius, y+0.88*radius);
  cairo_line_to (priv->cr, x+0.82*radius, y+0.76*radius);
  cairo_fill_preserve (priv->cr);
  cairo_stroke (priv->cr);
  cairo_set_line_width (priv->cr, 0.01 * radius);
  if (!priv->color_mode_inv)
		cairo_set_source_rgb (priv->cr, 0.1, 0.1, 0.1);
  else
      cairo_set_source_rgb (priv->cr, 0.9, 0.9, 0.9);  
  cairo_move_to (priv->cr, x+0.88*radius, y+0.82*radius);
  cairo_line_to (priv->cr, x+0.76*radius, y+0.82*radius);
  cairo_fill_preserve (priv->cr);
  cairo_stroke (priv->cr);  
  cairo_move_to (priv->cr, x+0.82*radius, y+0.88*radius);
  cairo_line_to (priv->cr, x+0.82*radius, y+0.76*radius);
  cairo_fill_preserve (priv->cr);
  cairo_stroke (priv->cr); 
  cairo_pattern_destroy (pat);
  return;
}

static void gtk_altimeter_draw_digital (GtkWidget * alt)
{
  GtkAltimeterPrivate *priv;
  if (gtk_altimeter_debug)
  {
    g_debug ("===> gtk_altimeter_draw_digital()");
  }
  g_return_if_fail (IS_GTK_ALTIMETER (alt));

  priv = GTK_ALTIMETER_GET_PRIVATE (alt);

  double x = priv->x;
  double y = priv->y;
  double radius = priv->radius;
  char str[GTK_ALTIMETER_MAX_STRING];
  int altitu = priv->altitude * 1000;

  //digital aff
  cairo_set_line_width (priv->cr, 1);
  cairo_rectangle (priv->cr, x - 0.145 * radius, y - 0.29 * radius - 0.165 * radius, 0.145 * radius, 0.18 * radius);
  cairo_rectangle (priv->cr, x - 0.29 * radius, y - 0.29 * radius - 0.165 * radius, 0.145 * radius, 0.18 * radius);
  cairo_rectangle (priv->cr, x - 0.435 * radius, y - 0.29 * radius - 0.165 * radius, 0.145 * radius, 0.18 * radius);
  cairo_rectangle (priv->cr, x + 0 * radius, y - 0.29 * radius - 0.165 * radius, 0.145 * radius, 0.18 * radius);
  cairo_rectangle (priv->cr, x + 0.145 * radius, y - 0.29 * radius - 0.165 * radius, 0.145 * radius, 0.18 * radius);
  cairo_rectangle (priv->cr, x + 0.29 * radius, y - 0.29 * radius - 0.165 * radius, 0.145 * radius, 0.18 * radius);
  if (!priv->color_mode_inv)
    cairo_set_source_rgb (priv->cr, 0.8, 0.8, 0.8);
  else
    cairo_set_source_rgb (priv->cr, 1 - 0.8, 1 - 0.8, 1 - 0.8);
  cairo_stroke (priv->cr);

  cairo_move_to (priv->cr, x - 0.427 * radius, y - 0.29 * radius);      // X00000
  sprintf (str, "%d", altitu / 100000000 % 10);
  cairo_show_text (priv->cr, str);
  cairo_move_to (priv->cr, x - 0.282 * radius, y - 0.29 * radius);      // 0X0000
  sprintf (str, "%d", altitu / 10000000 % 10);
  cairo_show_text (priv->cr, str);
  cairo_move_to (priv->cr, x - 0.137 * radius, y - 0.29 * radius);      // 00X000
  sprintf (str, "%d", altitu / 1000000 % 10);
  cairo_show_text (priv->cr, str);
  cairo_move_to (priv->cr, x + 0.008 * radius, y - 0.29 * radius);      // 000X00
  sprintf (str, "%d", altitu / 100000 % 10);
  cairo_show_text (priv->cr, str);
  cairo_move_to (priv->cr, x + 0.153 * radius, y - 0.29 * radius);      // 0000X0
  sprintf (str, "%d", altitu / 10000 % 10);
  cairo_show_text (priv->cr, str);
  cairo_move_to (priv->cr, x + 0.298 * radius, y - 0.29 * radius);      // 00000X
  sprintf (str, "%d", altitu / 1000 % 10);
  cairo_show_text (priv->cr, str);
  cairo_stroke (priv->cr);
  return;
}


static void gtk_altimeter_draw_hands (GtkWidget * alt)
{
  GtkAltimeterPrivate *priv;
  if (gtk_altimeter_debug)
  {
    g_debug ("===> gtk_altimeter_draw_hands()");
  }
  g_return_if_fail (IS_GTK_ALTIMETER (alt));

  priv = GTK_ALTIMETER_GET_PRIVATE (alt);

  double x = priv->x;
  double y = priv->y;
  double radius = priv->radius;
  int factor = 1;
  int altitu = priv->altitude * 1000;

  // 10 thousand hand
  cairo_save (priv->cr);
  if (!priv->color_mode_inv)
    cairo_set_source_rgb (priv->cr, 1, 1, 1);
  else
    cairo_set_source_rgb (priv->cr, 0, 0, 0);
  cairo_set_line_width (priv->cr, 2);
  cairo_move_to (priv->cr, x, y);
  cairo_line_to (priv->cr, x + radius * sin (5 * M_PI / 25
                                             * (altitu / (10000000 / factor) % 10) + M_PI / 50
                                             * (altitu / (1000000 / factor) % 10) + M_PI / 500
                                             * (altitu / (100000 / factor) % 10) + M_PI / 5000
                                             * (altitu / (10000 / factor) % 10) + M_PI / 50000
                                             * (altitu / (1000 / factor) % 10)),
                 y + radius * -cos (5 * M_PI / 25
                                    * (altitu / (10000000 / factor) % 10) + M_PI / 50
                                    * (altitu / (1000000 / factor) % 10) + M_PI / 500
                                    * (altitu / (100000 / factor) % 10) + M_PI / 5000
                                    * (altitu / (10000 / factor) % 10) + M_PI / 50000
                                    * (altitu / (1000 / factor) % 10)));

  cairo_move_to (priv->cr, x + (radius - 0.1 * radius) * sin (5 * M_PI / 25
                                                              * (altitu / (10000000 / factor) % 10) + M_PI / 50
                                                              * (altitu / (1000000 / factor) % 10) + M_PI / 500
                                                              * (altitu / (100000 / factor) % 10) + M_PI / 5000
                                                              * (altitu / (10000 / factor) % 10) + M_PI / 50000
                                                              * (altitu / (1000 / factor) % 10)),
                 y + (radius - 0.1 * radius) * -cos (5 * M_PI / 25
                                                     * (altitu / (10000000 / factor) % 10) + M_PI / 50
                                                     * (altitu / (1000000 / factor) % 10) + M_PI / 500
                                                     * (altitu / (100000 / factor) % 10) + M_PI / 5000
                                                     * (altitu / (10000 / factor) % 10) + M_PI / 50000
                                                     * (altitu / (1000 / factor) % 10)));
  cairo_line_to (priv->cr, x + radius * sin (M_PI / 50 + 5 * M_PI / 25
                                             * (altitu / (10000000 / factor) % 10) + M_PI / 50
                                             * (altitu / (1000000 / factor) % 10) + M_PI / 500
                                             * (altitu / (100000 / factor) % 10) + M_PI / 5000
                                             * (altitu / (10000 / factor) % 10) + M_PI / 50000
                                             * (altitu / (1000 / factor) % 10)),
                 y + radius * -cos (M_PI / 50 + 5 * M_PI / 25
                                    * (altitu / (10000000 / factor) % 10) + M_PI / 50
                                    * (altitu / (1000000 / factor) % 10) + M_PI / 500
                                    * (altitu / (100000 / factor) % 10) + M_PI / 5000
                                    * (altitu / (10000 / factor) % 10) + M_PI / 50000
                                    * (altitu / (1000 / factor) % 10)));
  cairo_line_to (priv->cr, x + radius * sin (-M_PI / 50 + 5 * M_PI / 25
                                             * (altitu / (10000000 / factor) % 10) + M_PI / 50
                                             * (altitu / (1000000 / factor) % 10) + M_PI / 500
                                             * (altitu / (100000 / factor) % 10) + M_PI / 5000
                                             * (altitu / (10000 / factor) % 10) + M_PI / 50000
                                             * (altitu / (1000 / factor) % 10)),
                 y + radius * -cos (-M_PI / 50 + 5 * M_PI / 25
                                    * (altitu / (10000000 / factor) % 10) + M_PI / 50
                                    * (altitu / (1000000 / factor) % 10) + M_PI / 500
                                    * (altitu / (100000 / factor) % 10) + M_PI / 5000
                                    * (altitu / (10000 / factor) % 10) + M_PI / 50000
                                    * (altitu / (1000 / factor) % 10)));
  cairo_move_to (priv->cr, x + (radius - 0.1 * radius) * sin (5 * M_PI / 25
                                                              * (altitu / (10000000 / factor) % 10) + M_PI / 50
                                                              * (altitu / (1000000 / factor) % 10) + M_PI / 500
                                                              * (altitu / (100000 / factor) % 10) + M_PI / 5000
                                                              * (altitu / (10000 / factor) % 10) + M_PI / 50000
                                                              * (altitu / (1000 / factor) % 10)),
                 y + (radius - 0.1 * radius) * -cos (5 * M_PI / 25
                                                     * (altitu / (10000000 / factor) % 10) + M_PI / 50
                                                     * (altitu / (1000000 / factor) % 10) + M_PI / 500
                                                     * (altitu / (100000 / factor) % 10) + M_PI / 5000
                                                     * (altitu / (10000 / factor) % 10) + M_PI / 50000
                                                     * (altitu / (1000 / factor) % 10)));
  cairo_fill_preserve (priv->cr);
  cairo_stroke (priv->cr);
  cairo_restore (priv->cr);

  // thousand hand
  cairo_save (priv->cr);
  if (!priv->color_mode_inv)
    cairo_set_source_rgb (priv->cr, 1, 1, 1);
  else
    cairo_set_source_rgb (priv->cr, 0, 0, 0);
  cairo_set_line_width (priv->cr, 0.03 * radius);
  cairo_move_to (priv->cr, x, y);

  cairo_line_to (priv->cr, x + (radius - 0.7 * radius) * sin (5 * M_PI / 25
                                                              * (altitu / (1000000 / factor) % 10) + M_PI / 50
                                                              * (altitu / (100000 / factor) % 10) + M_PI / 500
                                                              * (altitu / (10000 / factor) % 10) + M_PI / 5000
                                                              * (altitu / (1000 / factor) % 10) + M_PI / 50000
                                                              * (altitu / (100 / factor) % 10)),
                 y + (radius - 0.7 * radius) * -cos (5 * M_PI / 25
                                                     * (altitu / (1000000 / factor) % 10) + M_PI / 50
                                                     * (altitu / (100000 / factor) % 10) + M_PI / 500
                                                     * (altitu / (10000 / factor) % 10) + M_PI / 5000
                                                     * (altitu / (1000 / factor) % 10) + M_PI / 50000
                                                     * (altitu / (100 / factor) % 10)));
  cairo_stroke (priv->cr);
  cairo_restore (priv->cr);

  cairo_save (priv->cr);
  if (!priv->color_mode_inv)
    cairo_set_source_rgb (priv->cr, 1, 1, 1);
  else
    cairo_set_source_rgb (priv->cr, 0, 0, 0);
  cairo_set_line_width (priv->cr, 1);
  cairo_move_to (priv->cr, x, y);
  cairo_line_to (priv->cr, x + radius / 3 * sin (M_PI / 15 + 5 * M_PI / 25
                                                 * (altitu / (1000000 / factor) % 10) + M_PI / 50
                                                 * (altitu / (100000 / factor) % 10) + M_PI / 500
                                                 * (altitu / (10000 / factor) % 10) + M_PI / 5000
                                                 * (altitu / (1000 / factor) % 10) + M_PI / 50000
                                                 * (altitu / (100 / factor) % 10)),
                 y + radius / 3 * -cos (M_PI / 15 + 5 * M_PI / 25
                                        * (altitu / (1000000 / factor) % 10) + M_PI / 50
                                        * (altitu / (100000 / factor) % 10) + M_PI / 500
                                        * (altitu / (10000 / factor) % 10) + M_PI / 5000
                                        * (altitu / (1000 / factor) % 10) + M_PI / 50000
                                        * (altitu / (100 / factor) % 10)));
  cairo_line_to (priv->cr, x + radius / 2 * sin (5 * M_PI / 25
                                                 * (altitu / (1000000 / factor) % 10) + M_PI / 50
                                                 * (altitu / (100000 / factor) % 10) + M_PI / 500
                                                 * (altitu / (10000 / factor) % 10) + M_PI / 5000
                                                 * (altitu / (1000 / factor) % 10) + M_PI / 50000
                                                 * (altitu / (100 / factor) % 10)),
                 y + radius / 2 * -cos (5 * M_PI / 25
                                        * (altitu / (1000000 / factor) % 10) + M_PI / 50
                                        * (altitu / (100000 / factor) % 10) + M_PI / 500
                                        * (altitu / (10000 / factor) % 10) + M_PI / 5000
                                        * (altitu / (1000 / factor) % 10) + M_PI / 50000
                                        * (altitu / (100 / factor) % 10)));
  cairo_line_to (priv->cr, x + radius / 3 * sin (-M_PI / 15 + 5 * M_PI / 25
                                                 * (altitu / (1000000 / factor) % 10) + M_PI / 50
                                                 * (altitu / (100000 / factor) % 10) + M_PI / 500
                                                 * (altitu / (10000 / factor) % 10) + M_PI / 5000
                                                 * (altitu / (1000 / factor) % 10) + M_PI / 50000
                                                 * (altitu / (100 / factor) % 10)),
                 y + radius / 3 * -cos (-M_PI / 15 + 5 * M_PI / 25
                                        * (altitu / (1000000 / factor) % 10) + M_PI / 50
                                        * (altitu / (100000 / factor) % 10) + M_PI / 500
                                        * (altitu / (10000 / factor) % 10) + M_PI / 5000
                                        * (altitu / (1000 / factor) % 10) + M_PI / 50000
                                        * (altitu / (100 / factor) % 10)));
  cairo_line_to (priv->cr, x, y);
  cairo_fill_preserve (priv->cr);
  cairo_stroke (priv->cr);
  cairo_restore (priv->cr);

  cairo_save (priv->cr);
  if (!priv->color_mode_inv)
    cairo_set_source_rgb (priv->cr, 0, 0, 0);
  else
    cairo_set_source_rgb (priv->cr, 1, 1, 1);
  cairo_arc (priv->cr, x, y, radius - 0.86 * radius, M_PI / 3 + 5 * M_PI / 25
             * (altitu / (1000000 / factor) % 10) + M_PI / 50
             * (altitu / (100000 / factor) % 10) + M_PI / 500
             * (altitu / (10000 / factor) % 10) + M_PI / 5000
             * (altitu / (1000 / factor) % 10) + M_PI / 50000
             * (altitu / (100 / factor) % 10),
             2 * M_PI / 3 + 5 * M_PI / 25
             * (altitu / (1000000 / factor) % 10) + M_PI / 50
             * (altitu / (100000 / factor) % 10) + M_PI / 500
             * (altitu / (10000 / factor) % 10) + M_PI / 5000
             * (altitu / (1000 / factor) % 10) + M_PI / 50000 * (altitu / (100 / factor) % 10));
  cairo_line_to (priv->cr, x, y);
  cairo_fill_preserve (priv->cr);
  cairo_stroke (priv->cr);
  cairo_restore (priv->cr);

  // hundred hand
  if (factor == 100)
  {
    cairo_save (priv->cr);
    if (!priv->color_mode_inv)
      cairo_set_source_rgb (priv->cr, 1, 1, 1);
    else
      cairo_set_source_rgb (priv->cr, 0, 0, 0);
    cairo_set_line_width (priv->cr, 0.03 * radius);
    cairo_move_to (priv->cr, x, y);
    cairo_line_to (priv->cr, x + (radius - 0.2 * radius) * sin (5 * M_PI / 25
                                                                * (altitu / (100000 / factor) % 10) + M_PI / 50
                                                                * (altitu / (10000 / factor) % 10) + M_PI / 500
                                                                * (altitu / (1000 / factor) % 10) + M_PI / 5000
                                                                * (altitu / (100 / factor) % 10)),
                   y + (radius - 0.2 * radius) * -cos (5 * M_PI / 25
                                                       * (altitu / (100000 / factor) % 10) + M_PI / 50
                                                       * (altitu / (10000 / factor) % 10) + M_PI / 500
                                                       * (altitu / (1000 / factor) % 10) + M_PI / 5000
                                                       * (altitu / (100 / factor) % 10)));
    cairo_stroke (priv->cr);
    cairo_restore (priv->cr);

    cairo_save (priv->cr);
    if (!priv->color_mode_inv)
      cairo_set_source_rgb (priv->cr, 0, 0, 0);
    else
      cairo_set_source_rgb (priv->cr, 1, 1, 1);
    cairo_set_line_width (priv->cr, 0.03 * radius);
    cairo_move_to (priv->cr, x - (radius - 0.8 * radius) * sin (5 * M_PI / 25
                                                                * (altitu / (100000 / factor) % 10) + M_PI / 50
                                                                * (altitu / (10000 / factor) % 10) + M_PI / 500
                                                                * (altitu / (1000 / factor) % 10) + M_PI / 5000
                                                                * (altitu / (100 / factor) % 10)),
                   y - (radius - 0.8 * radius) * -cos (5 * M_PI / 25
                                                       * (altitu / (100000 / factor) % 10) + M_PI / 50
                                                       * (altitu / (10000 / factor) % 10) + M_PI / 500
                                                       * (altitu / (1000 / factor) % 10) + M_PI / 5000
                                                       * (altitu / (100 / factor) % 10)));
    cairo_arc (priv->cr, x - (radius - 0.8 * radius) * sin (5 * M_PI / 25
                                                            * (altitu / (100000 / factor) % 10) + M_PI / 50
                                                            * (altitu / (10000 / factor) % 10) + M_PI / 500
                                                            * (altitu / (1000 / factor) % 10) + M_PI / 5000
                                                            * (altitu / (100 / factor) % 10)),
               y - (radius - 0.8 * radius) * -cos (5 * M_PI / 25
                                                   * (altitu / (100000 / factor) % 10) + M_PI / 50
                                                   * (altitu / (10000 / factor) % 10) + M_PI / 500
                                                   * (altitu / (1000 / factor) % 10) + M_PI / 5000
                                                   * (altitu / (100 / factor) % 10)),
               radius - 0.98 * radius, 0, 2 * M_PI);
    cairo_move_to (priv->cr, x - (radius - 0.8 * radius) * sin (5 * M_PI / 25
                                                                * (altitu / (100000 / factor) % 10) + M_PI / 50
                                                                * (altitu / (10000 / factor) % 10) + M_PI / 500
                                                                * (altitu / (1000 / factor) % 10) + M_PI / 5000
                                                                * (altitu / (100 / factor) % 10)),
                   y - (radius - 0.8 * radius) * -cos (5 * M_PI / 25
                                                       * (altitu / (100000 / factor) % 10) + M_PI / 50
                                                       * (altitu / (10000 / factor) % 10) + M_PI / 500
                                                       * (altitu / (1000 / factor) % 10) + M_PI / 5000
                                                       * (altitu / (100 / factor) % 10)));
    cairo_line_to (priv->cr, x, y);
    cairo_stroke (priv->cr);
    cairo_restore (priv->cr);
  }
  else
  {
    cairo_save (priv->cr);
    if (!priv->color_mode_inv)
      cairo_set_source_rgb (priv->cr, 1, 1, 1);
    else
      cairo_set_source_rgb (priv->cr, 0, 0, 0);
    cairo_set_line_width (priv->cr, 0.03 * radius);
    cairo_move_to (priv->cr, x, y);
    cairo_line_to (priv->cr, x + (radius - 0.2 * radius) * sin (5 * M_PI / 25
                                                                * (altitu / (100000 / factor) % 10) + M_PI / 50
                                                                * (altitu / (10000 / factor) % 10) + M_PI / 500
                                                                * (altitu / (1000 / factor) % 10) + M_PI / 5000
                                                                * (altitu / (100 / factor) % 10) + M_PI / 50000
                                                                * (altitu / (10 / factor) % 10)),
                   y + (radius - 0.2 * radius) * -cos (5 * M_PI / 25
                                                       * (altitu / (100000 / factor) % 10) + M_PI / 50
                                                       * (altitu / (10000 / factor) % 10) + M_PI / 500
                                                       * (altitu / (1000 / factor) % 10) + M_PI / 5000
                                                       * (altitu / (100 / factor) % 10) + M_PI / 50000
                                                       * (altitu / (10 / factor) % 10)));
    cairo_stroke (priv->cr);
    cairo_restore (priv->cr);

    cairo_save (priv->cr);
    if (!priv->color_mode_inv)
      cairo_set_source_rgb (priv->cr, 0, 0, 0);
    else
      cairo_set_source_rgb (priv->cr, 1, 1, 1);
    cairo_set_line_width (priv->cr, 0.03 * radius);
    cairo_move_to (priv->cr, x - (radius - 0.8 * radius) * sin (5 * M_PI / 25
                                                                * (altitu / (100000 / factor) % 10) + M_PI / 50
                                                                * (altitu / (10000 / factor) % 10) + M_PI / 500
                                                                * (altitu / (1000 / factor) % 10) + M_PI / 5000
                                                                * (altitu / (100 / factor) % 10) + M_PI / 50000
                                                                * (altitu / (10 / factor) % 10)),
                   y - (radius - 0.8 * radius) * -cos (5 * M_PI / 25
                                                       * (altitu / (100000 / factor) % 10) + M_PI / 50
                                                       * (altitu / (10000 / factor) % 10) + M_PI / 500
                                                       * (altitu / (1000 / factor) % 10) + M_PI / 5000
                                                       * (altitu / (100 / factor) % 10) + M_PI / 50000
                                                       * (altitu / (10 / factor) % 10)));
    cairo_arc (priv->cr, x - (radius - 0.8 * radius) * sin (5 * M_PI / 25
                                                            * (altitu / (100000 / factor) % 10) + M_PI / 50
                                                            * (altitu / (10000 / factor) % 10) + M_PI / 500
                                                            * (altitu / (1000 / factor) % 10) + M_PI / 5000
                                                            * (altitu / (100 / factor) % 10) + M_PI / 50000
                                                            * (altitu / (10 / factor) % 10)),
               y - (radius - 0.8 * radius) * -cos (5 * M_PI / 25
                                                   * (altitu / (100000 / factor) % 10) + M_PI / 50
                                                   * (altitu / (10000 / factor) % 10) + M_PI / 500
                                                   * (altitu / (1000 / factor) % 10) + M_PI / 5000
                                                   * (altitu / (100 / factor) % 10) + M_PI / 50000
                                                   * (altitu / (10 / factor) % 10)),
               radius - 0.98 * radius, 0, 2 * M_PI);
    cairo_move_to (priv->cr, x - (radius - 0.8 * radius) * sin (5 * M_PI / 25
                                                                * (altitu / (100000 / factor) % 10) + M_PI / 50
                                                                * (altitu / (10000 / factor) % 10) + M_PI / 500
                                                                * (altitu / (1000 / factor) % 10) + M_PI / 5000
                                                                * (altitu / (100 / factor) % 10) + M_PI / 50000
                                                                * (altitu / (10 / factor) % 10)),
                   y - (radius - 0.8 * radius) * -cos (5 * M_PI / 25
                                                       * (altitu / (100000 / factor) % 10) + M_PI / 50
                                                       * (altitu / (10000 / factor) % 10) + M_PI / 500
                                                       * (altitu / (1000 / factor) % 10) + M_PI / 5000
                                                       * (altitu / (100 / factor) % 10) + M_PI / 50000
                                                       * (altitu / (10 / factor) % 10)));
    cairo_line_to (priv->cr, x, y);
    cairo_stroke (priv->cr);
    cairo_restore (priv->cr);
  }

  // centre cercle 
  if (!priv->color_mode_inv)
    cairo_set_source_rgb (priv->cr, 1, 1, 1);
  else
    cairo_set_source_rgb (priv->cr, 0, 0, 0);
  cairo_arc (priv->cr, x, y, radius - 0.98 * radius, 0, 2 * M_PI);
  cairo_fill_preserve (priv->cr);
  cairo_stroke (priv->cr);

  return;
}

static gboolean gtk_altimeter_button_press_event (GtkWidget * widget, GdkEventButton * ev)
{
  GtkAltimeterPrivate *priv;
  gint x = 0, y = 0;

  if (gtk_altimeter_debug)
  {
    g_debug ("===> gtk_altimeter_button_press_event_cb()");
  }
  g_return_val_if_fail (IS_GTK_ALTIMETER (widget), FALSE);

  priv = GTK_ALTIMETER_GET_PRIVATE (widget);

  if ((ev->type & GDK_BUTTON_PRESS) && (ev->button == 1))
  {
    gdk_window_get_pointer (ev->window, &x, &y, &priv->mouse_state);
    priv->mouse_pos.x = x;
    priv->mouse_pos.y = y;
    return TRUE;
  }
  if ((ev->type & GDK_BUTTON_PRESS) && (ev->button == 2) && priv->b_mouse_onoff)
  {
    gtk_altimeter_debug = gtk_altimeter_debug ? FALSE : TRUE;
    return TRUE;
  }
  if ((ev->type & GDK_BUTTON_PRESS) && (ev->button == 3))
  {
    priv->b_mouse_onoff = priv->b_mouse_onoff ? FALSE : TRUE;
    return TRUE;
  }

  return FALSE;
}

static gboolean gtk_altimeter_motion_notify_event (GtkWidget * widget, GdkEventMotion * ev)
{
  GtkAltimeterPrivate *priv;
  GdkModifierType state;
  gint x = 0, y = 0;

  if (gtk_altimeter_debug)
  {
    g_debug ("===> gtk_altimeter_motion_notify_event_cb()");
  }
  g_return_val_if_fail (IS_GTK_ALTIMETER (widget), FALSE);

  priv = GTK_ALTIMETER_GET_PRIVATE (widget);

  if (ev->is_hint)
  {
    gdk_window_get_pointer (ev->window, &x, &y, &state);
  }
  else
  {
    x = ev->x;
    y = ev->y;
    state = ev->state;
  }

  /* save mousse coordinates */
  priv->mouse_pos.x = x;
  priv->mouse_pos.y = y;
  priv->mouse_state = state;

  if (gtk_altimeter_debug)
  {
    g_debug ("===> gtk_altimeter_motion_notify_event_cb() : mouse x=%d, y=%d", x, y);
  }

  return TRUE;
}

static void gtk_altimeter_destroy (GtkObject * object)
{
  GtkAltimeterPrivate *priv = NULL;
  GtkWidget *widget = NULL;

  if (gtk_altimeter_debug)
  {
    g_debug ("===> gtk_altimeter_destroy(enter)");
  }

  g_return_if_fail (object != NULL);

  widget = GTK_WIDGET (object);

  g_return_if_fail (IS_GTK_ALTIMETER (widget));

  priv = GTK_ALTIMETER_GET_PRIVATE (widget);
  g_return_if_fail (priv != NULL);

  if (priv->cr)
  {
    g_free (priv->cr);

    if (GTK_OBJECT_CLASS (gtk_altimeter_parent_class)->destroy != NULL)
    {
      (*GTK_OBJECT_CLASS (gtk_altimeter_parent_class)->destroy) (object);
    }
  }
  if (gtk_altimeter_debug)
  {
    g_debug ("gtk_altimeter_destroy(exit)");
  }

  return;
}

static void gtk_altimeter_set_property (GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec)
{
  GtkAltimeterPrivate *priv = NULL;
  GtkAltimeter *alt = NULL;

  if (gtk_altimeter_debug)
  {
    g_debug ("===> gtk_altimeter_set_property()");
  }
  g_return_if_fail (object != NULL);

  alt = GTK_ALTIMETER (object);
  g_return_if_fail (IS_GTK_ALTIMETER (alt));

  priv = GTK_ALTIMETER_GET_PRIVATE (alt);
  g_return_if_fail (priv != NULL);

  switch (prop_id)
  {
    case PROP_INVERSED_COLOR:
      priv->color_mode_inv = g_value_get_boolean (value);
      break;
    case PROP_UNIT_IS_FEET:
      priv->unit_is_feet = g_value_get_boolean (value);
      break;
    case PROP_UNIT_STEP_VALUE:
      priv->unit_value = g_value_get_int (value);
      break;
    case PROP_RADIAL_COLOR:
      priv->radial_color = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
  return;
}
