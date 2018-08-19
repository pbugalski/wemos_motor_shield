## wemos_motor_shield_firmwere
This is an alternative firmware for the [Wemos Motor Shield](https://wiki.wemos.cc/products:d1_mini_shields:motor_shield).
The i2c protocol implemented in this firmware differs for the original in order to overcome some limitaions.
I made a compatible Arduino library [here](https://github.com/danielfmo/WEMOS_Motor_Shield_Arduino_Library).

## How to build UNIX:
 * `sudo apt-get install gcc-arm-none-eabi stm32flash`
 * `git clone <this repository>`
 * `make`
 * `sudo make flash`

## How to build WINDOWS:
 * Install [MinGW](https://sourceforge.net/projects/mingw/files/latest/download)
 * Install [Stm32flash](https://sourceforge.net/projects/stm32flash)
 * `git clone <this repository>`
 * `make`
 * `make flash`
 * [Useful guide to accomplish the above](https://www.jann.cc/2013/10/10/embedded_development_with_open_source_tools_on_windows.html#installing-mingw-and-msys)

## How to flash firmware
 * The recomended method is by the use of a USBtoTTL adapter and connecting the RTS pin of the shield to Vcc.
 * Please follow ꝺeshipu's Hackaday [projecthttps://hackaday.io/project/18439-motor-shield-reprogramming?page=1#discussion-list)] for details on how to flash this firmware.

## License
This project and all files in this repository are released under MIT License, see the [LICENSE](LICENSE.md) file for more details.

## Credits
This project is based on the firmware released by [pbugalski](https://github.com/pbugalski/wemos_motor_shield) and thank him for that.
This project is as alternative to the original firmware for the [WEMOS motor shield](https://github.com/wemos/Motor_Shield_Firmware).
I would like to thank all Hackaday community for their support and specifically [ꝺeshipu](https://hackaday.io/deshipu) for his [project](https://hackaday.io/project/18439-motor-shield-reprogramming?page=1#discussion-list).