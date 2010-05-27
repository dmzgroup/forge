#!/bin/sh

. ../../../scripts/envsetup.sh

$RUN_DEBUG$BIN_HOME/dmzAppQt -f config/uploader.xml config/render.xml config/common.xml config/input.xml $*
