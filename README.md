# Shopno Calendar

<p>
  <img alt="Xfce 4.18+" src="https://img.shields.io/badge/Xfce-4.18%2B-2284f2?style=for-the-badge&labelColor=2b2b33&logo=xfce&logoColor=white">
  <a href="COPYING"><img alt="License: GPL-2.0-or-later" src="https://img.shields.io/badge/license-GPL--2.0+-6b7280?style=for-the-badge&labelColor=2b2b33"></a>
</p>

A clock and calendar flyout for the Xfce panel.

> [!NOTE]
> Shopno Calendar is in early development. It shows the time and date on the
> panel and a calendar when clicked; more of the flyout is on the way.

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

**Calendar flyout.** Click the clock for the time, large, the full date,
and the month, with today as a filled square. The week starts on the day
your locale uses, or on Monday, Sunday or Saturday if you choose. The
weekend is tinted: Saturday and Sunday, Friday and Saturday as in
Bangladesh and much of the Middle East, Friday only, Sunday only, or
none. ISO 8601 week numbers can be shown left of the days. The arrows or
the mouse wheel change the month; click the title to pick a month of the
year, and again for a year of the decade. From the keyboard, the arrow
keys move between days, Page Up and Page Down change the month, and Home
goes back to today. Every day is named for screen readers. The flyout
opens next to the clock on any panel edge, and closes on Escape, a click
outside or another click on the clock. It takes every color from your
GTK theme, light or dark; rules in `~/.config/gtk-3.0/gtk.css` can
override its style.

## Installing

Download the package for your distribution from the
[latest release](https://github.com/fa-saikat/xfce4-shopno-calendar-plugin/releases/latest),
then install it:

| Distribution             | File                         | Install with                                                |
|--------------------------|------------------------------|-------------------------------------------------------------|
| Debian 13                | `*.debian13_amd64.deb`       | `sudo apt install ./xfce4-shopno-calendar-plugin_*.deb`     |
| Ubuntu 24.04, Mint 22    | `*.ubuntu24.04_amd64.deb`    | `sudo apt install ./xfce4-shopno-calendar-plugin_*.deb`     |
| Fedora                   | `*.fc*.x86_64.rpm`           | `sudo dnf install ./xfce4-shopno-calendar-plugin-*.rpm`     |
| Arch Linux               | `*.pkg.tar.zst`              | `sudo pacman -U xfce4-shopno-calendar-plugin-*.pkg.tar.zst` |

Each release also has a `SHA256SUMS` file to check the downloads with
`sha256sum -c --ignore-missing SHA256SUMS`.

Then restart the panel (`xfce4-panel -r`), right-click it, choose
**Panel > Add New Items…**, and add **Shopno Calendar**.

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
- **[GTK](https://gitlab.gnome.org/GNOME/gtk)** (LGPL-2.1-or-later): the
  locale's first day of the week is read from glibc the way `GtkCalendar`
  does it.
- **[Shopno Menu](https://github.com/fa-saikat/xfce4-shopno-menu-plugin)**:
  the flyout opens, grabs input and closes the same way its menu does.
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

The icon in `data/icons/shopno-calendar.svg` is the ShopnoOS logo,
© 2026 JaduPc, licensed under the GNU General Public License v3.0 or later. It
stands in until Shopno Calendar has a logo of its own. ShopnoOS Linux is
a trademark of JaduPc.
