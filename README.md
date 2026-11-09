# Shopno Calendar

<p>
  <img alt="Xfce 4.18+" src="https://img.shields.io/badge/Xfce-4.18%2B-2284f2?style=for-the-badge&labelColor=2b2b33&logo=xfce&logoColor=white">
  <a href="COPYING"><img alt="License: GPL-2.0-or-later" src="https://img.shields.io/badge/license-GPL--2.0+-6b7280?style=for-the-badge&labelColor=2b2b33"></a>
</p>

A clock and calendar flyout for the Xfce panel.

> [!NOTE]
> Shopno Calendar is in early development. It shows the time and date on the
> panel; the calendar flyout is not there yet.

## Why this exists

I'm building it for [ShopnoOS](https://github.com/fa-saikat/shopno-os), next to
[Shopno Menu](https://github.com/fa-saikat/xfce4-shopno-menu-plugin) and
[Shopno Quick Settings](https://github.com/fa-saikat/xfce4-shopno-quicksettings-plugin).
The calendar that comes with Xfce's Clock plugin is a plain `GtkCalendar` and
looks out of place next to them.

It works on any distribution with Xfce 4.18 or newer.

## Features

**Panel clock.** The time over the date, like Windows 11, or one line, the
time only, or your own format with the same codes as Xfce's Clock plugin.
12 or 24-hour time follows your locale until you pick one, and seconds are
optional. The clock changes exactly when the minute does, and shows the
right time straight after a resume, a clock change or a new time zone.
On a vertical panel the text is turned; on a deskbar the date and time
are stacked. Change it in *Properties*, from the clock's right-click menu,
with a live preview.

## Building

Dependencies: Meson 0.62+, a C compiler, gettext, GTK 3.24+, and the
development files for libxfce4panel, libxfce4ui, libxfce4util and xfconf
4.18+.

On Debian and Ubuntu:

```sh
sudo apt install build-essential meson ninja-build gettext pkg-config \
  libgtk-3-dev libxfce4panel-2.0-dev libxfce4ui-2-dev libxfce4util-dev libxfconf-0-dev
```

Then:

```sh
meson setup build --prefix=/usr
meson compile -C build
meson test -C build
sudo meson install -C build
xfce4-panel -r
```

Then add **Shopno Calendar** to the panel from *Panel Preferences → Items*.

## Credits

Shopno Calendar builds on:

- **Xfce**: the [panel plugin API](https://gitlab.xfce.org/xfce/xfce4-panel),
  [xfconf](https://gitlab.xfce.org/xfce/xfconf) for settings, and
  [libxfce4ui](https://gitlab.xfce.org/xfce/libxfce4ui) and
  [libxfce4util](https://gitlab.xfce.org/xfce/libxfce4util), all on
  [GTK 3](https://gtk.org/).
- **[Xfce's Clock plugin](https://gitlab.xfce.org/xfce/xfce4-panel/-/tree/master/plugins/clock)**
  (GPL-2.0-or-later): the list of format codes that need the clock to
  update every second comes from its `clock-time.c` by Andrzej.
- Projects whose calendars it learned from, without sharing code:
  [elementary's Date & Time indicator](https://github.com/elementary/wingpanel-indicator-datetime),
  [NiceCalendar](https://github.com/MSTAURUS/NiceCalendar) by MSTAURUS and
  [AxisClock](https://github.com/Axis0S/xfce4-axisclock-plugin) by
  Kamil Nowicki.
- **Windows 11**, whose calendar flyout it borrows ideas from.

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md).

## License

Shopno Calendar is released under the
[GNU General Public License v2.0 or later](COPYING).
