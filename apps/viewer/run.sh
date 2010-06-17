#!/bin/sh

. ../../../scripts/envsetup.sh

$RUN_DEBUG$BIN_HOME/dmzAppQt -f config/render.xml config/viewer.xml config/pick.xml config/runtime.xml config/common.xml config/input.xml config/js.xml config/resource.xml config/spectator-overlay.xml config/archive.xml $*
