#!/bin/bash

### Global variables ###

CURRENT_KERNEL=$(uname --kernel-release)
ROOT=""
PM=""
PM_ATTRIBUTES=""
DEPENDENCIES=""
COMMON_DEPENDENCIES="gawk tar git gcc bc make zenity"
UPDATE_PROCEDURE=""
INSTALL_PROCEDURE=""
REMOVE_PROCEDURE="" # Can break packages. Will not use unless necessary.

# Colours
RED="\x1b[1;31m"
GREEN="\x1b[1;32m"
YELLOW="\x1b[1;33m"
BLUE="\x1b[1;34m"
PURPLE="\x1b[1;35m"
CYAN="\x1b[1;36m"
RESET_COLOUR="\x1b[0m"

### Functions ###

# int: Checks if script is running with root.
#      If not, no updates will happen.
#      The script will compile but will not install.
AM_I_ROOT()
{
  local user=$(whoami)

  case $user in
  root) return 0 ;;
  *) return 1 ;;
  esac
}

# void: Get the root manager name.
#       If no root manager is found, "sh -c" will be used.
#       Therefore the user will need to insert the password for every
#         root dependable command.
GET_ROOT_MANAGER()
{
  if type sudo &>/dev/null; then ROOT="sudo"
  elif type doas &>/dev/null; then ROOT="doas"
  else ROOT="su -c"
  fi
}

# int: Check kernel version.
CHECK_KERNEL_VERSION()
{
  local KERNEL_VERSION=$(printf "${CURRENT_KERNEL}" | cut --delimiter="." --fields=1-2)
  local KERNEL_MAJOR_VERSION=$(printf "${KERNEL_VERSION}" | cut --delimiter="." --fields=1)
  local KERNEL_MINOR_VERSION=$(printf "${KERNEL_VERSION}" | cut --delimiter="." --fields=2)

  # If kernel is greater or equal to 6.3.x.
  if [ ${KERNEL_MAJOR_VERSION} -ge "6" ]
    then if [ ${KERNEL_MINOR_VERSION} -ge "3" ]
      then return 0
      else return 1
    fi
  else return 1
  fi
}

# int: Return the package manager name.
CHECK_PACKAGE_MANAGER()
{
  if type dpkg &>/dev/null; then return 1
  elif type pacman &>/dev/null; then return 2
  elif type dnf &>/dev/null; then return 3
  else return 0
  fi
}

# void: Get the package manager information.
GET_PM_INFO()
{
  CHECK_PACKAGE_MANAGER
  local i=$?

  case $i in
  0)
    printf "${YELLOW}""Warning: Package manager not found. Skipping dependency check...""${RESET_COLOUR}""\n"
    exit 1
    ;;
  1)
    PM="apt-get"
    PM_ATTRIBUTES="-y"
    UPDATE_PROCEDURE="update"
    INSTALL_PROCEDURE="install"
    REMOVE_PROCEDURE="--purge remove"
    DEPENDENCIES="linux-headers-${CURRENT_KERNEL}"
    ;;
  2)
    PM="pacman"
    PM_ATTRIBUTES="--needed --noconfirm"
    UPDATE_PROCEDURE="-Sy"
    INSTALL_PROCEDURE="-S"
    REMOVE_PROCEDURE="-Rdd"
    DEPENDENCIES="linux-headers"
    ;;
  3)
    PM="dnf"
    PM_ATTRIBUTES="-y"
    UPDATE_PROCEDURE="check-update"
    INSTALL_PROCEDURE="install"
    DEPENDENCIES="kernel-devel"
    ;;
  esac
}

IS_TERMUX()
{
  if [ $TERMUX_VERSION ]
    then return 0
    else return 1
  fi
}

### Check if it is running from Termux ###

if IS_TERMUX
  then
    printf "${RED}""Error: Termux is not supported yet. Consider compiling the repository without script execution.""${RESET_COLOUR}""\n"
    exit 1
fi

### Check about root ###

CURRENT_HOME=""
if ! AM_I_ROOT
  then
    printf "${YELLOW}""Warning: Script running without root privileges. If no recognized root manager is found, the script will use \"su -c\" instead...""${RESET_COLOUR}""\n"
    GET_ROOT_MANAGER
    CURRENT_HOME="/home/$(whoami)/"
  else CURRENT_HOME="/root/"
fi

### Find about package availability ###

# Apply driver name based on the kernel version.
CURRENT_DRIVER=""
if CHECK_KERNEL_VERSION
  then CURRENT_DRIVER="rtl8xxxu"
  else CURRENT_DRIVER="r8188eu"
fi
NEW_DRIVER="8188eu"

# Get package manager information.
GET_PM_INFO

# Update and install provided packages.
${ROOT} ${PM} ${UPDATE_PROCEDURE} && ${ROOT} ${PM} ${PM_ATTRIBUTES} ${INSTALL_PROCEDURE} ${DEPENDENCIES} ${COMMON_DEPENDENCIES}

### Proceed with the compiling ### 

# Environment variables.
CURRENT_SHELL=$(ps -hp $$ | awk '{printf $5}')
CURRENT_PWD=$(pwd)
CORE=$(nproc)
IS_NOT_BLACKLISTED=$(cat /etc/modprobe.d/realtek.conf | grep "blacklist ${CURRENT_DRIVER}")
INSTALLATION_PATH_DOES_NOT_EXIST=$(printf $PATH | grep /usr/local/bin)
DO_DIRECTORY_EXISTS=0
REPOSITORY_HOME="../rtl8188eus/"

# Check if the repository already exists.
if ! [ -d ${REPOSITORY_HOME} ]
  then if ! [ -d ${CURRENT_HOME}/.local/share/git/rtl8188eus ]
    then
      REPOSITORY_HOME="${CURRENT_HOME}/.local/share/git/rtl8188eus/"
      mkdir -pv ${REPOSITORY_HOME}
      git clone --recursive https://gitlab.com/KanuX/rtl8188eus.git "${REPOSITORY_HOME}"
    else
      printf "${YELLOW}""Warning: Repository already exists.""${RESET_COLOUR}""\n"
      REPOSITORY_HOME="${CURRENT_HOME}/.local/share/git/rtl8188eus/"
  fi
  else printf "${YELLOW}""Warning: Repository already exists.""${RESET_COLOUR}""\n"
fi
cd ${REPOSITORY_HOME}

# Check if the device is already blacklisted.
if [ -z "${IS_NOT_BLACKLISTED}" ]; then
  ${ROOT} mkdir -pv /etc/modprobe.d/
  printf "blacklist ${CURRENT_DRIVER}\n" | ${ROOT} tee /etc/modprobe.d/realtek.conf
fi

# Check if /usr/local/bin/ exists.
if [ -z "${INSTALLATION_PATH_DOES_NOT_EXIST}" ]; then
  ${ROOT} mkdir -pv /usr/local/bin/
  printf "export PATH=/usr/local/bin:$PATH\n" >> ${CURRENT_HOME}/."${CURRENT_SHELL}""rc"
fi

# Installation process.
${ROOT} modprobe -r ${CURRENT_DRIVER} # This one can be anything. I am using this one because is the common TL-WN722N v2/v3
${ROOT} modprobe -r ${NEW_DRIVER} 2&>/dev/null # Remove old/installed module. Visual purposes.
make -j${CORE} && ${ROOT} make install
${ROOT} modprobe ${NEW_DRIVER} # Load the new driver/module
${ROOT} cp -v scripts/toggle-monitor.sh /usr/local/bin/toggle-monitor
${ROOT} chown ${USER}:${USER} /usr/local/bin/toggle-monitor
${ROOT} chmod +x /usr/local/bin/toggle-monitor
${ROOT} cp -v rtl8188eus-toggle-monitor.desktop /usr/share/applications

# Check if compiled and installed.
if ! ls /usr/lib/modules/${CURRENT_KERNEL}/kernel/drivers/net/wireless/ | grep ${NEW_DRIVER}
  then
    printf "${RED}""The driver could not be installed. Please post the log under the \"issues\" tab.""${RESET_COLOUR}""\n"
    exit 1
  else
    printf "${GREEN}""The driver/module has been installed!""${RESET_COLOUR}""\n"
    notify-send "The driver/module has been installed!"
    exit 0
fi
