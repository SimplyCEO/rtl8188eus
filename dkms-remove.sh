#!/bin/sh

if [ ${EUID} -ne 0 ]; then
  printf "You must run this with root privileges.\n" 2>&1
  exit 1
else
  printf "Running dkms install...\n"
fi

DRV_DIR=rtl8188eus
DRV_NAME=8188eu
DRV_VERSION=5.3.9

dkms remove ${DRV_NAME}/${DRV_VERSION} --all
rm -rf /usr/src/${DRV_NAME}-${DRV_VERSION}
RESULT=$?

if [ "${RESULT}" != "0" ]; then
  printf "An error has occured while trying to remove the driver.\n" 2>&1
else
  printf "Driver succesfully uninstalled.\n"
fi

exit ${RESULT}
