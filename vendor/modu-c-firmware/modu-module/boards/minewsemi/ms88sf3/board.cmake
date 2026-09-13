# Copyright (c) 2026 EKS Inc.
# Created by Ryu.
# SPDX-License-Identifier: LicenseRef-EKS-NonCommercial-1.0

board_runner_args(pyocd "--target=nrf52840" "--frequency=1000000")
board_runner_args(nrfjprog "--nrf-family=NRF52" "--softreset")
board_runner_args(jlink "--device=nRF52840_xxAA" "--speed=4000")
include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/nrfjprog.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
