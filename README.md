# Game Boy RW, stm32f4 side

This is the microcontroller side of my Game Boy reader/writer.

The code is written in C and uses the [libopencm3](https://github.com/libopencm3/libopencm3) library to control the STM32F4.  I have used a NUCLEO-F411RE, but other STM32 microcontroller should work too.

Supported commands are:

- Reading ROM
- Reading/Writing SRAM
- Writing ROM on flash Chinese cartridges

## Building

```
cd src
make gb-rw.stlink-flash
```

## Debugging

```
openocd
```

```
arm-none-eabi-gdb gb-rw.elf
target extended-remote :3333
c
```

## Details

You can read more about how reprogramming Game Boy Chinese cartridge works in my blog post:

- [Writing Game Boy Chinese cartridges with an STM32F4](https://dhole.github.io/post/gameboy_cartridge_rw_1/)

## License

The code is released under the 3-clause BSD License.
