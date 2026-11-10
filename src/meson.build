resources = gnome.compile_resources(
  'shopno-cal-resources',
  'resources/shopno-calendar.gresource.xml',
  source_dir: 'resources',
  c_name: 'shopno_cal',
)

shopno_cal_deps = [
  gtk,
  gio_unix,
  libxfce4panel,
  libxfce4ui,
  libxfce4util,
  libxfconf,
]

# Everything except the panel glue, shared with the tests and the
# preview tool
shopno_cal_core = static_library(
  'shopno-cal-core',
  sources: [
    'shopno-cal-clock.c',
    'shopno-cal-dialog.c',
    'shopno-cal-flyout.c',
    'shopno-cal-format.c',
    'shopno-cal-month.c',
    'shopno-cal-settings.c',
    resources,
  ],
  include_directories: config_inc,
  dependencies: shopno_cal_deps,
  gnu_symbol_visibility: 'hidden',
  pic: true,
)

shared_module(
  'shopno-calendar',
  sources: [
    'shopno-cal-plugin.c',
  ],
  include_directories: config_inc,
  dependencies: shopno_cal_deps,
  link_whole: shopno_cal_core,
  gnu_symbol_visibility: 'hidden',
  install: true,
  install_dir: libdir / 'xfce4' / 'panel' / 'plugins',
)

shopno_cal_inc = include_directories('.')
