#!/bin/bash

# Check kernel version.
function check_if_new()
{
  local KERNEL_VERSION=$(uname --kernel-release | cut --delimiter="." --fields=1-2)
  local KERNEL_MAJOR_VERSION=$(echo ${KERNEL_VERSION} | cut --delimiter="." --fields=1)
  local KERNEL_MINOR_VERSION=$(echo ${KERNEL_VERSION} | cut --delimiter="." --fields=2)

  if [ ${KERNEL_MAJOR_VERSION} -ge "6" ]; then
    if [ ${KERNEL_MINOR_VERSION} -ge "3" ]; then
      return 0
    else
      return 1
    fi
  else
    return 1
  fi
}

# Apply driver name based on the kernel version.
CURRENT_DRIVER=""
if check_if_new; then
  CURRENT_DRIVER="rtl8xxxu"
else
  CURRENT_DRIVER="r8188eu"
fi
NEW_DRIVER="8188eu"

# Check the package manager.
if type dpkg &>/dev/null; then
	sudo apt-get update && sudo apt-get install gawk tar git gcc bc make linux-headers-$(uname -r) zenity -y
elif type pacman &>/dev/null; then
	sudo pacman -S --needed --noconfirm gawk tar git gcc bc make linux-headers zenity
elif type dnf &>/dev/null; then
	sudo dnf install -y gawk tar git gcc bc make kernel-devel zenity
else
	printf "${GREEN}""Consider installing the kernel headers by yourself.""${RESET_COLOUR}""\n"
  exit 1
fi

# Environment variables
CURRENT_SHELL=$(ps -hp $$ | awk '{printf $5}')
CURRENT_USER=$(whoami)
CURRENT_HOME=""
CURRENT_PWD=$(pwd)
CORE=$(nproc)
IS_NOT_BLACKLISTED=$(cat /etc/modprobe.d/realtek.conf | grep "blacklist ${CURRENT_DRIVER}")
INSTALLATION_PATH_DOES_NOT_EXIST=$(printf $PATH | grep /usr/local/sbin)
DO_DIRECTORY_EXISTS=0

# Colours
RED="\e[1;31m"
GREEN="\e[1;32m"
YELLOW="\e[1;33m"
BLUE="\e[1;34m"
PURPLE="\e[1;35m"
CYAN="\e[1;36m"
RESET_COLOUR="\e[0m"

# Set home based on user or root
if [ ${CURRENT_USER} == "root" ]; then
	CURRENT_HOME="/root"
else
	CURRENT_HOME="/home/${CURRENT_USER}"
fi

# Check if the repository already exists
if ! [ -d ../rtl8188eus ]; then
	if ! [ -d ${CURRENT_HOME}/rtl8188eus ]; then
		git clone --recursive https://gitlab.com/KanuX/rtl8188eus.git
	else
		printf "${YELLOW}""[!] Repository already exists.""${RESET_COLOUR}""\n"
	fi
	cd "${CURRENT_HOME}/rtl8188eus"
fi

# Check if the device is already blacklisted.
if [ -z "${IS_NOT_BLACKLISTED}" ]; then
	sudo mkdir -pv /etc/modprobe.d
	printf "blacklist ${CURRENT_DRIVER}\n" | sudo tee "/etc/modprobe.d/realtek.conf"
fi

# Check if /usr/local/sbin exists.
if [ -z "${INSTALLATION_PATH_DOES_NOT_EXIST}" ]; then
	sudo mkdir -pv /usr/local/sbin
	printf "export PATH=/usr/local/sbin:$PATH\n" >> ."${CURRENT_SHELL}""rc"
fi

# Installation process.
sudo rmmod ${CURRENT_DRIVER} # This one can be anything. I am using this one because is the common TL-WN722N v2/v3
sudo rmmod ${NEW_DRIVER} 2&>/dev/null # Remove old/installed module. Visual purposes.
make -j${CORE} && sudo make install
sudo modprobe ${NEW_DRIVER} # Load the new driver/module
sudo cp -v scripts/toggle-monitor.sh /usr/local/bin/toggle-monitor
sudo chown ${USER}:${USER} /usr/local/bin/toggle-monitor
sudo chmod +x /usr/local/bin/toggle-monitor
sudo cp -v rtl8188eus-toggle-monitor.desktop /usr/share/applications
printf "${GREEN}""The driver/module has been installed!""${RESET_COLOUR}""\n"
notify-send "The driver/module has been installed!"
