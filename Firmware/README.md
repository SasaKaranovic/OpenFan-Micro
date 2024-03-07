# How to build firmware

For our firmware development, we are going to use Arduino. Mainly because it's user friendly, easy to use and most hobbyists are familiar with it or have already used it. While we could build the same or even more efficient firmware using C and ESP-IDF, we are going to stick with Arduino and hope that this allows the project to be
more friendly and easier to use/modify by wider DIY/hacker community.

# Do I really need to build the firmware?

If you don not need any firmware modifications and you are using the official [OpenFAN Micro](https://shop.sasakaranovic.com/products/openfan-micro), then you do not have to. You can just use the web flasher to update your board to the latest firmware version.
Latest version of the firmware is available online and can be loaded using the [web flasher](https://docs.sasakaranovic.com/openfan_micro/firmware_upgrade/).
Please keep in mind that this firmware version is for the official OpenFAN Micro that is available on the [web shop](https://shop.sasakaranovic.com/products/openfan-micro).

## How can I build/compile firmware?

Arduino has many third party libraries that you can use and also build firmware for many different boards.
However, depending for which board you are building firmware and also which libraries your project (sketch) uses, you might need to manually install them or download them trough board/library manager.

While above mentioned solution works fine, it can lead to issues like installing the "wrong/different" version of the library, different board files, "wrong/different" build/upload tools and so on.
On top of that, since board files and libraries are shared, we could install a library or a board file while working on another project that breaks our current project.
Hopefully you can see why this is not so great for open-source open-hardware project where we want everyone to be able to easily checkout a repository and hit the ground running without spending hours setting up and debugging what library or board files are missing.

Luckily we can automate/avoid this by using a build system like PlatformIO. So below we will have two ways of building the firmware, one will be "easy" by leveraging PlatformiIO build system, and the other one will be a manual one by using Arduino IDE and manually searching and downloading all the board/library files.

## - The super easy way - Compile and upload using PlatformIO build system  - 

### 1. Install PlatformIO

Installing PlatformIO CLI is pretty straight-forward and also well documented for Windows, Linux and MacOS.
You will need to follow few steps and get PlatformIO CLI installed, detailed tutorial can be found at https://platformio.org/install/cli
Make sure to install [PlatformIO Core](https://docs.platformio.org/en/latest//core/installation.html#installation-methods 'https://docs.platformio.org/en/latest//core/installation.html#installation-methods') and allso that it is available trough [shell](https://docs.platformio.org/en/latest//core/installation.html#piocore-install-shell-commands 'PlatformIO Core - Install Shell Commands').

### 2. Build firmware

Open shell/command-prompt and navigate to 'Firmware/platformio' folder.
1. Compile the firmware by typing `pio run`, PlatformIO will download all the required board files and libraries and finally compile the firmware.
2. Upload the firmware with `pio run --target upload --upload-port <COM-PORT>`. Make sure to replace `<COM-PORT>` with your ESP32's COM port (ie COM1 or /dev/ttyACM0)


[<- Go back to repository root](../README.md)
