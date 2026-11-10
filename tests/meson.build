test(
  'format',
  executable(
    'test-format',
    'test-format.c',
    include_directories: [config_inc, shopno_cal_inc],
    dependencies: shopno_cal_deps,
    link_with: shopno_cal_core,
  ),
  env: ['LC_ALL=C.UTF-8', 'TZ=UTC'],
)

test(
  'month',
  executable(
    'test-month',
    'test-month.c',
    include_directories: [config_inc, shopno_cal_inc],
    dependencies: shopno_cal_deps,
    link_with: shopno_cal_core,
  ),
  env: ['LC_ALL=C.UTF-8'],
)
