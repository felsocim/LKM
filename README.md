# LKM

Basic examples of Linux kernel modules

## About

This repository contains the sources of two simple modules for Linux kernels developed for my assignement for Embedded Systems courses at the [University of Strasbourg](http://www.unistra.fr). I hope these examples will help someone who wants to discover the basics of drivers development for Linux systems.

## Modules

### Hello world

This module is very simple. It does nothing special but prints the message "Hello world!" onto the kernel ring buffer whenever the module is loaded to the system and the message "Bye, cruel world." whenever it is unloaded from the system.

### FIFO

This module aimes to simulate a pipe device which one can use to send a stream of characters to, then read the same stream of characters from in FIFO (First In First Out) mode.

## Usage

All the following commands need to be executed as superuser.

### Acquiring Linux kernel headers

To build a module, you will need to acquire your current kernel's header files. The corresponding package is called *linux-headers-x.x.x-\**. For example, on Debian-based systems like Ubuntu, it can be installed using the following command:

`apt install linux-headers-$(uname -r)`

Note that, the **uname** command allows to retrieve current kernel version.

### Building modules

Finally, to build a module, you have to navigate to the module's root folder and run the following command from:

`make -C /usr/src/linux-headers-$(uname -r) M=$(pwd) modules`

The module binary file produced has the **.ko** extension.

### Load and test

To load a module, fire the following command:

`insmod <module file name>.ko`

To verify whether the module was successfully loaded, use:

`lsmod | grep '<module name>'`

#### Testing the **Hello world** module

To be able to see the welcome and the bye message printed by this module, simply observe the output of the **dmesg** command.

#### Testing the **FIFO** module

First, send a stream of characters to the device created by this module called **/dev/fifo** using, for example, the **echo** command as follows:

`echo 'Hi, this is me!!!' > /dev/fifo`

Finally, read the stream of characters from the device using the **cat** command as follows:

`cat /dev/fifo`

## Author

[Marek Felsoci](mailto:marek.felsoci@gmail.com) - Master's degree student at the [University of Strasbourg](http://www.unistra.fr), France

## License

This sources are licensed under the terms of the General Public License version 2.0. For details, see the [license file](LICENSE) in this repository.
