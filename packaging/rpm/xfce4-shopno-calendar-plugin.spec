Name:           xfce4-shopno-calendar-plugin
Version:        1.0.0
Release:        1%{?dist}
Summary:        Clock and calendar flyout for the Xfce panel

License:        GPL-2.0-or-later
URL:            https://github.com/fa-saikat/xfce4-shopno-calendar-plugin
Source0:        %{url}/archive/v%{version}/%{name}-%{version}.tar.gz

BuildRequires:  gcc
BuildRequires:  gettext
BuildRequires:  meson
BuildRequires:  pkgconfig(gio-unix-2.0)
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(gtk+-3.0)
BuildRequires:  pkgconfig(libxfce4panel-2.0)
BuildRequires:  pkgconfig(libxfce4ui-2)
BuildRequires:  pkgconfig(libxfce4util-1.0)
BuildRequires:  pkgconfig(libxfconf-0)

Requires:       xfce4-panel
Suggests:       picom

%description
Shopno Calendar is a panel clock for Xfce with a calendar flyout, in the
spirit of the Windows 11 clock.

The clock shows the time over the date, on one line, the time only, or a
custom format, and updates exactly on the minute and straight after a
resume, a clock change or a new time zone. Clicking it opens a calendar
with the time, the full date and the month, which can be moved through by
mouse, wheel and keyboard. Under the calendar are the selected day's
details, or the days between two dates, and a countdown timer; the footer
copies the date or time and shows whether the clock is synced. It takes
every color from the GTK theme, and can blur the background with picom.

%prep
%autosetup

%build
%meson -Dpreview=false
%meson_build

%install
%meson_install
# Add %%find_lang and "%%files -f %%{name}.lang" once po/LINGUAS lists a
# language. Until then rpm rejects the empty list.

%files
%license COPYING
%doc README.md extras/picom/shopno-calendar.conf
%{_libdir}/xfce4/panel/plugins/libshopno-calendar.so
%{_datadir}/xfce4/panel/plugins/shopno-calendar.desktop
%{_datadir}/icons/hicolor/scalable/apps/shopno-calendar.svg

%changelog
* Thu Nov 12 2026 FA Saikat <fahimabrar.saikat@gmail.com> - 1.0.0-1
- First stable release
- Calendar options: the first day of the week, the weekend days (including
  Friday and Saturday) and ISO 8601 week numbers
- Day details under the calendar, and the days between two dates with
  Shift-click or Shift and the arrow keys, with working days
- Full date, week number and day of the year in the panel clock's tooltip
- Copy button for the date or time: ISO, local or long date, date and
  time, or Unix time
- Clock sync status from systemd-timedated, and a button for the system's
  Date & Time settings
- Countdown timer with presets or any length up to 4 hours, remembered
  between sessions. It shows the time left on the panel and ends with a
  desktop notification that can restart it
- Optional background blur with picom; the rules are in
  extras/picom/shopno-calendar.conf
- The seconds in the flyout header are the same size as the time
- A broken translation of a date format is handled safely, and the
  flyout no longer relays out unchanged text every second

* Wed Nov 11 2026 FA Saikat <fahimabrar.saikat@gmail.com> - 0.1.0-1
- Initial release
- Panel clock: time over date, one line, time only or a custom format,
  12 or 24-hour time and optional seconds; updates on the minute and
  straight after a resume, a clock change or a new time zone
- Properties dialog with a live preview
- Calendar flyout with the time, the full date and the month, today as a
  filled square, in the GTK theme's colors
- Navigation by arrows, mouse wheel and keyboard, with month and year
  views; every day has an accessible name
- Turned text on vertical panels and a stacked clock on deskbars
- ShopnoOS logo as the icon until the plugin has its own
