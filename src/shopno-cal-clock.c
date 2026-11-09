/*
 * Copyright (C) 2026 FA Saikat
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

/*
 * Waking up on time
 *
 * A GLib timeout counts on the monotonic clock, which stops while the
 * computer sleeps and ignores changes to the wall clock. After a resume or
 * an NTP correction the panel would show the wrong time until the next
 * tick. Xfce's Clock plugin works around this by listening to logind's
 * PrepareForSleep and checking for drift after every tick.
 *
 * On Linux a timerfd on CLOCK_REALTIME, armed for the absolute time of the
 * next second or minute, does all of that: it fires on the wall clock,
 * fires at once after a resume when its time has passed, and with
 * TFD_TIMER_CANCEL_ON_SET returns ECANCELED when the clock is set. Other
 * systems fall back to a timeout, recomputed after every tick.
 */

#include "config.h"

#include <errno.h>
#include <string.h>
#include <unistd.h>

#include <gio/gio.h>
#include <glib-unix.h>

#ifdef HAVE_TIMERFD
#include <sys/timerfd.h>
#endif

#include "shopno-cal-clock.h"

#define LOCALTIME_PATH "/etc/localtime"

/* timedatectl replaces /etc/localtime in more than one step */
#define TIMEZONE_RELOAD_DELAY 250

enum
{
  TICK,
  N_SIGNALS
};

struct _ShopnoCalClock
{
  GObject       __parent__;

  guint         interval;

  gint          timer_fd;
  guint         timer_watch_id;
  guint         timeout_id;

  GTimeZone    *timezone;
  GFileMonitor *timezone_monitor;
  guint         timezone_reload_id;
};

static guint signals[N_SIGNALS];

G_DEFINE_TYPE (ShopnoCalClock, shopno_cal_clock, G_TYPE_OBJECT)

static void shopno_cal_clock_arm (ShopnoCalClock *self);



/*
 * Time zone
 */

/* GLib caches the local time zone for the life of the process, so read
 * the zone /etc/localtime points to, as timedatectl sets it. A TZ
 * variable wins, as it does for every other program. */
static GTimeZone *
shopno_cal_clock_load_timezone (void)
{
  GTimeZone   *timezone = NULL;
  gchar       *target;
  const gchar *name;

  if (g_getenv ("TZ") != NULL)
    return g_time_zone_new_local ();

  target = g_file_read_link (LOCALTIME_PATH, NULL);
  if (target != NULL)
    {
      name = strstr (target, "zoneinfo/");
      if (name != NULL)
        timezone = g_time_zone_new_identifier (name + strlen ("zoneinfo/"));
      g_free (target);
    }

  return timezone != NULL ? timezone : g_time_zone_new_local ();
}



static gboolean
shopno_cal_clock_reload_timezone (gpointer data)
{
  ShopnoCalClock *self = data;
  GTimeZone      *timezone;

  self->timezone_reload_id = 0;

  timezone = shopno_cal_clock_load_timezone ();
  if (g_strcmp0 (g_time_zone_get_identifier (timezone),
                 g_time_zone_get_identifier (self->timezone)) == 0)
    {
      g_time_zone_unref (timezone);
      return G_SOURCE_REMOVE;
    }

  g_time_zone_unref (self->timezone);
  self->timezone = timezone;

  g_signal_emit (self, signals[TICK], 0);

  return G_SOURCE_REMOVE;
}



static void
shopno_cal_clock_localtime_changed (GFileMonitor      *monitor,
                                    GFile             *file,
                                    GFile             *other_file,
                                    GFileMonitorEvent  event,
                                    ShopnoCalClock    *self)
{
  if (self->timezone_reload_id != 0)
    g_source_remove (self->timezone_reload_id);

  self->timezone_reload_id = g_timeout_add (TIMEZONE_RELOAD_DELAY,
                                            shopno_cal_clock_reload_timezone, self);
}



static void
shopno_cal_clock_watch_timezone (ShopnoCalClock *self)
{
  GFile  *file;
  GError *error = NULL;

  if (g_getenv ("TZ") != NULL)
    return;

  file = g_file_new_for_path (LOCALTIME_PATH);
  self->timezone_monitor = g_file_monitor_file (file, G_FILE_MONITOR_NONE, NULL, &error);
  g_object_unref (file);

  if (self->timezone_monitor == NULL)
    {
      g_debug ("Not following time zone changes: %s", error->message);
      g_error_free (error);
      return;
    }

  g_signal_connect (self->timezone_monitor, "changed",
                    G_CALLBACK (shopno_cal_clock_localtime_changed), self);
}



/*
 * Timer
 */

static void
shopno_cal_clock_tick (ShopnoCalClock *self)
{
  g_signal_emit (self, signals[TICK], 0);
  shopno_cal_clock_arm (self);
}



#ifdef HAVE_TIMERFD
static gboolean
shopno_cal_clock_timer_ready (gint         fd,
                              GIOCondition condition,
                              gpointer     data)
{
  guint64 expirations;

  /* ECANCELED means the clock was set; tick to show the new time */
  if (read (fd, &expirations, sizeof (expirations)) < 0 && errno == EAGAIN)
    return G_SOURCE_CONTINUE;

  shopno_cal_clock_tick (data);

  return G_SOURCE_CONTINUE;
}
#endif



static gboolean
shopno_cal_clock_timeout (gpointer data)
{
  ShopnoCalClock *self = data;

  self->timeout_id = 0;
  shopno_cal_clock_tick (self);

  return G_SOURCE_REMOVE;
}



static void
shopno_cal_clock_arm (ShopnoCalClock *self)
{
  gint64 now = g_get_real_time ();
  gint64 step = (gint64) self->interval * G_USEC_PER_SEC;
  gint64 next = (now / step + 1) * step;

  if (self->timeout_id != 0)
    {
      g_source_remove (self->timeout_id);
      self->timeout_id = 0;
    }

#ifdef HAVE_TIMERFD
  if (self->timer_fd >= 0)
    {
      struct itimerspec spec = { 0 };

      spec.it_value.tv_sec = next / G_USEC_PER_SEC;
      spec.it_value.tv_nsec = (next % G_USEC_PER_SEC) * 1000;

      if (timerfd_settime (self->timer_fd, TFD_TIMER_ABSTIME | TFD_TIMER_CANCEL_ON_SET, &spec, NULL) == 0)
        return;

      /* Set while arming: the next read reports it, so arm once more
       * without asking to be told */
      if (errno == ECANCELED
          && timerfd_settime (self->timer_fd, TFD_TIMER_ABSTIME, &spec, NULL) == 0)
        return;

      g_warning ("Failed to set the clock timer, falling back to a timeout: %s", g_strerror (errno));
      g_source_remove (self->timer_watch_id);
      self->timer_watch_id = 0;
      close (self->timer_fd);
      self->timer_fd = -1;
    }
#endif

  /* One millisecond late rather than early, so the new minute is shown */
  self->timeout_id = g_timeout_add_full (G_PRIORITY_HIGH, (next - now) / 1000 + 1,
                                         shopno_cal_clock_timeout, self, NULL);
}



/*
 * GObject
 */

static void
shopno_cal_clock_dispose (GObject *object)
{
  ShopnoCalClock *self = SHOPNO_CAL_CLOCK (object);

  g_clear_handle_id (&self->timeout_id, g_source_remove);
  g_clear_handle_id (&self->timer_watch_id, g_source_remove);
  g_clear_handle_id (&self->timezone_reload_id, g_source_remove);

  if (self->timer_fd >= 0)
    {
      close (self->timer_fd);
      self->timer_fd = -1;
    }

  if (self->timezone_monitor != NULL)
    {
      g_signal_handlers_disconnect_by_data (self->timezone_monitor, self);
      g_file_monitor_cancel (self->timezone_monitor);
      g_clear_object (&self->timezone_monitor);
    }

  G_OBJECT_CLASS (shopno_cal_clock_parent_class)->dispose (object);
}



static void
shopno_cal_clock_finalize (GObject *object)
{
  ShopnoCalClock *self = SHOPNO_CAL_CLOCK (object);

  g_time_zone_unref (self->timezone);

  G_OBJECT_CLASS (shopno_cal_clock_parent_class)->finalize (object);
}



static void
shopno_cal_clock_class_init (ShopnoCalClockClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = shopno_cal_clock_dispose;
  object_class->finalize = shopno_cal_clock_finalize;

  signals[TICK] = g_signal_new (g_intern_static_string ("tick"),
                                G_TYPE_FROM_CLASS (klass),
                                G_SIGNAL_RUN_LAST,
                                0, NULL, NULL, NULL,
                                G_TYPE_NONE, 0);
}



static void
shopno_cal_clock_init (ShopnoCalClock *self)
{
  self->interval = SHOPNO_CAL_CLOCK_MINUTE;
  self->timer_fd = -1;
  self->timezone = shopno_cal_clock_load_timezone ();

#ifdef HAVE_TIMERFD
  self->timer_fd = timerfd_create (CLOCK_REALTIME, TFD_NONBLOCK | TFD_CLOEXEC);
  if (self->timer_fd >= 0)
    self->timer_watch_id = g_unix_fd_add_full (G_PRIORITY_HIGH, self->timer_fd, G_IO_IN,
                                               shopno_cal_clock_timer_ready, self, NULL);
  else
    g_warning ("Failed to create the clock timer, falling back to a timeout: %s", g_strerror (errno));
#endif

  shopno_cal_clock_watch_timezone (self);
  shopno_cal_clock_arm (self);
}



ShopnoCalClock *
shopno_cal_clock_new (void)
{
  return g_object_new (SHOPNO_TYPE_CAL_CLOCK, NULL);
}



void
shopno_cal_clock_set_interval (ShopnoCalClock *clock,
                               guint           seconds)
{
  g_return_if_fail (SHOPNO_IS_CAL_CLOCK (clock));
  g_return_if_fail (seconds > 0);

  if (clock->interval == seconds)
    return;

  clock->interval = seconds;
  shopno_cal_clock_arm (clock);
}



GDateTime *
shopno_cal_clock_get_now (ShopnoCalClock *clock)
{
  g_return_val_if_fail (SHOPNO_IS_CAL_CLOCK (clock), NULL);

  return g_date_time_new_now (clock->timezone);
}
