#!/bin/sh

. ../../../scripts/envsetup.sh

$RUN_DEBUG$BIN_HOME/dmzAppQt -f config/common.xml config/input.xml config/render.xml config/lua.xml $*
