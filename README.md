# Shopno Calendar

<p>
  <img alt="Xfce 4.18+" src="https://img.shields.io/badge/Xfce-4.18%2B-2284f2?style=for-the-badge&labelColor=2b2b33&logo=xfce&logoColor=white">
  <a href="COPYING"><img alt="License: GPL-2.0-or-later" src="https://img.shields.io/badge/license-GPL--2.0+-6b7280?style=for-the-badge&labelColor=2b2b33"></a>
</p>

A clock and calendar flyout for the Xfce panel.

> [!NOTE]
> Shopno Calendar is in early development and does not do anything useful yet.

## Why this exists

I'm building it for [ShopnoOS](https://github.com/fa-saikat/shopno-os), next to
[Shopno Menu](https://github.com/fa-saikat/xfce4-shopno-menu-plugin) and
[Shopno Quick Settings](https://github.com/fa-saikat/xfce4-shopno-quicksettings-plugin).
The calendar that comes with Xfce's Clock plugin is a plain `GtkCalendar` and
looks out of place next to them.

It works on any distribution with Xfce 4.18 or newer.

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
sudo meson install -C build
xfce4-panel -r
```

## Credits

Shopno Calendar builds on:

- **Xfce**: the [panel plugin API](https://gitlab.xfce.org/xfce/xfce4-panel),
  [xfconf](https://gitlab.xfce.org/xfce/xfconf) for settings, and
  [libxfce4ui](https://gitlab.xfce.org/xfce/libxfce4ui) and
  [libxfce4util](https://gitlab.xfce.org/xfce/libxfce4util), all on
  [GTK 3](https://gtk.org/).
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
