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

cp -r ../${DRV_DIR} /usr/src/${DRV_NAME}-${DRV_VERSION}

dkms add -m ${DRV_NAME} -v ${DRV_VERSION}
dkms build -m ${DRV_NAME} -v ${DRV_VERSION}
dkms install -m ${DRV_NAME} -v ${DRV_VERSION}
RESULT=$?

if [ "${RESULT}" != "0" ]; then
  printf "An error has occured while trying to install the driver.\n" 2>&1
else
  printf "Driver succesfully installed.\n"
fi

exit ${RESULT}
