# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(libssm)

target_sources(app PRIVATE ../src/ssm-io.c ../src/ssm-throw.c ../src/ssm-timer.c)
