#!/bin/bash
set -euo pipefail

ISEMU=${1:-0}

export DOCKER_BUILDKIT=1
docker  build  .  -t smoo-client-build
docker  run  --rm       \
  -u $(id -u):$(id -g)  \
  -v "/$PWD/":/app/     \
  -e ISEMU=${ISEMU}     \
  smoo-client-build     \
;
docker  rmi  smoo-client-build

# copy romfs
DIR=$(dirname ./starlight_patch_*/atmosphere/)
cp  -r  ./romfs/  $DIR/atmosphere/contents/0100000000010000/.

# create file structure for emulator builds
if [ "$ISEMU" -eq "1" ] ; then
  rm  -rf  $DIR/SMOO/
  mkdir  -p  $DIR/SMOO/
  mv  $DIR/atmosphere/contents/0100000000010000/exefs  $DIR/SMOO/exefs
  mv  $DIR/atmosphere/contents/0100000000010000/romfs  $DIR/SMOO/romfs
  mv  $DIR/atmosphere/exefs_patches/StarlightBase/3CA12DFAAF9C82DA064D1698DF79CDA1.ips  $DIR/SMOO/exefs/
  rm  -rf  $DIR/atmosphere/
fi
