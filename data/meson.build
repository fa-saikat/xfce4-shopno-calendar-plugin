i18n.merge_file(
  input: 'shopno-calendar.desktop.in',
  output: 'shopno-calendar.desktop',
  type: 'desktop',
  po_dir: meson.project_source_root() / 'po',
  install: true,
  install_dir: datadir / 'xfce4' / 'panel' / 'plugins',
)

# The ShopnoOS logo for now, until the plugin has its own icon
install_data(
  'icons/shopno-calendar.svg',
  install_dir: datadir / 'icons' / 'hicolor' / 'scalable' / 'apps',
)
