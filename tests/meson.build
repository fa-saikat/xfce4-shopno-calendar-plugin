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

test(
  'details',
  executable(
    'test-details',
    'test-details.c',
    include_directories: [config_inc, shopno_cal_inc],
    dependencies: shopno_cal_deps,
    link_with: shopno_cal_core,
  ),
  env: ['LC_ALL=C.UTF-8'],
)

# Runs a private D-Bus daemon with a fake timedated
test(
  'sync',
  executable(
    'test-sync',
    'test-sync.c',
    include_directories: [config_inc, shopno_cal_inc],
    dependencies: shopno_cal_deps,
    link_with: shopno_cal_core,
  ),
  env: ['LC_ALL=C.UTF-8'],
)

# Runs short timers in real time, and a private D-Bus daemon with a fake
# notification server
test(
  'timer',
  executable(
    'test-timer',
    'test-timer.c',
    include_directories: [config_inc, shopno_cal_inc],
    dependencies: shopno_cal_deps,
    link_with: shopno_cal_core,
  ),
  env: ['LC_ALL=C.UTF-8'],
)
