# SPDX-License-Identifier: MIT
# Keep the ORIGINAL UI layout, modifiers, USB/BT status, cat and layer widgets.
# No Apple logo or v3 dashboard. Only the battery widget is MODU-specific.
# All generated compatibility files go into the build directory, not upstream.
# v5: Do not silently reintroduce decorated widget containers via a config
# override. The upstream custom screen already sets its background/fonts.
foreach(_modu_theme MONO DEFAULT SIMPLE)
  if(CONFIG_LV_USE_THEME_${_modu_theme})
    message(FATAL_ERROR
      "CONFIG_LV_USE_THEME_${_modu_theme} adds unwanted borders/padding to the original custom display. Set it to n.")
  endif()
endforeach()
if(CONFIG_SHIELD_DONGLE_DISPLAY)
  message(FATAL_ERROR "Use modu_dongle_oled alone, not an additional dongle_display shield")
endif()
set(_dd_root "${ZEPHYR_ZMK_DONGLE_DISPLAY_MODULE_DIR}")
if(NOT EXISTS "${_dd_root}/boards/shields/dongle_display/custom_status_screen.c")
  set(_dd_root "${ZEPHYR_BASE}/../zmk-dongle-display")
endif()
set(_dd "${_dd_root}/boards/shields/dongle_display")
if(NOT EXISTS "${_dd}/custom_status_screen.c")
  message(FATAL_ERROR "Original UI module missing. Commit config/west.yml and run west update / a new GitHub build")
endif()
file(READ "${_dd}/widgets/modifiers_sym.c" _dd_symbols)
if(NOT _dd_symbols MATCHES "LV_COLOR_FORMAT_I1")
  message(FATAL_ERROR "This ZMK pin requires the LVGL 9 display branch, not v0.3")
endif()
file(READ "${_dd}/widgets/modifiers.c" _dd_mods)
if(NOT _dd_mods MATCHES "CONFIG_ZMK_DONGLE_DISPLAY_MAC_MODIFIERS" OR
   NOT _dd_mods MATCHES "cmd_icon" OR NOT _dd_mods MATCHES "opt_icon")
  message(FATAL_ERROR "Original Mac modifier API changed; review the display dependency")
endif()
execute_process(COMMAND git -C "${_dd_root}" rev-parse HEAD
                OUTPUT_VARIABLE _dd_rev OUTPUT_STRIP_TRAILING_WHITESPACE
                RESULT_VARIABLE _dd_git_result)
if(_dd_git_result EQUAL 0)
  message(STATUS "Original dongle display revision: ${_dd_rev}")
  file(WRITE "${PROJECT_BINARY_DIR}/modu-display-revision.txt" "${_dd_rev}\n")
endif()
set(_dd_generated "${PROJECT_BINARY_DIR}/modu-original-display")
file(MAKE_DIRECTORY "${_dd_generated}")
zephyr_library_include_directories("${_dd}" "${_dd}/widgets")

# Current display main uses the new ZMK_BATTERY spelling. The keyboard is
# intentionally kept on its working ZMK pin, which uses ZMK_BATTERY_REPORTING.
file(READ "${_dd}/custom_status_screen.c" _dd_screen)
string(REGEX REPLACE "CONFIG_ZMK_BATTERY([^A-Za-z0-9_]|$)"
       "CONFIG_ZMK_BATTERY_REPORTING\\1" _dd_screen "${_dd_screen}")
if(NOT _dd_screen MATCHES "zmk_widget_dongle_battery_status_init")
  message(FATAL_ERROR "Original battery widget API changed")
endif()
file(WRITE "${_dd_generated}/custom_status_screen.c" "${_dd_screen}")
zephyr_library_sources("${_dd_generated}/custom_status_screen.c")
zephyr_library_sources("${CMAKE_CURRENT_LIST_DIR}/src/dongle_status.c")
zephyr_library_sources("${_dd}/widgets/output_status.c" "${_dd}/widgets/output_status_sym.c")
if(CONFIG_ZMK_DONGLE_DISPLAY_MODIFIERS)
  zephyr_library_sources("${_dd}/widgets/modifiers.c" "${_dd}/widgets/modifiers_sym.c")
endif()
if(CONFIG_ZMK_DONGLE_DISPLAY_LAYER)
  zephyr_library_sources("${_dd}/widgets/layer_status.c")
endif()
if(CONFIG_ZMK_DONGLE_DISPLAY_BONGO_CAT)
  zephyr_library_sources("${_dd}/widgets/bongo_cat.c" "${_dd}/widgets/bongo_cat_images.c")
endif()
if(CONFIG_ZMK_DONGLE_DISPLAY_WPM)
  zephyr_library_sources("${_dd}/widgets/wpm_status.c" "${_dd}/widgets/wpm_status_sym.c")
endif()
if(CONFIG_ZMK_HID_INDICATORS)
  # ZMK initializes display listeners with a NULL event. Do not dereference it.
  file(READ "${_dd}/widgets/hid_indicators.c" _dd_hid)
  string(REPLACE ".hid_indicators = ev->indicators,"
                 ".hid_indicators = ev ? ev->indicators : 0,"
                 _dd_hid "${_dd_hid}")
  if(_dd_hid MATCHES "\\.hid_indicators[ \t]*=[ \t]*ev->indicators")
    message(FATAL_ERROR "Unprotected HID indicator initialization in upstream")
  endif()
  file(WRITE "${_dd_generated}/hid_indicators.c" "${_dd_hid}")
  zephyr_library_sources("${_dd_generated}/hid_indicators.c")
endif()
