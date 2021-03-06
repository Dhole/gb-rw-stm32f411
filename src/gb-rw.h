#ifndef GBLINK_H
#define GBLINK_H

#include <libopencm3/stm32/gpio.h>

#define GPIO_0_7  (GPIO0 | GPIO1 | GPIO2 | GPIO3 | GPIO4 | GPIO5 | GPIO6 | GPIO7)
#define GPIO_8_15 (GPIO8 | GPIO9 | GPIO10 | GPIO11 | GPIO12 | GPIO13 | GPIO14 | GPIO15)
#define GPIO_8_13 (GPIO8 | GPIO9 | GPIO10 | GPIO11 | GPIO12 | GPIO13)

// GPIO configuration

#define GPIOP_SIGNAL GPIOA

#define GPIOP_CLK  GPIOA
#define GPION_CLK  GPIO10

#define GPIOP_WR  GPIOA
#define GPION_WR  GPIO6

#define GPIOP_RD  GPIOA
#define GPION_RD  GPIO7

#define GPIOP_CS  GPIOA
#define GPION_CS  GPIO8

#define GPIOP_RESET  GPIOA
#define GPION_RESET  GPIO9

#define GPIOP_CS2  GPIOA
#define GPION_CS2  GPIO9

#define GPIOP_DATA  GPIOB

#define GPIOP_ADDR   GPIOC
#define GPIOP_ADDR_14_15  GPIOA
#define GPION_ADDR_14 GPIO0
#define GPION_ADDR_15 GPIO1

#define GPIOP_GBA_ROM_DATA_0_13  GPIOC
#define GPIOP_GBA_ROM_DATA_14_15 GPIOA
#define GPION_GBA_ROM_DATA_14    GPIO0
#define GPION_GBA_ROM_DATA_15    GPIO1

#define GPIOP_GBA_SRAM_DATA  GPIOB

#define GPIOP_GBA_ADDR_0_13  GPIOC
#define GPIOP_GBA_ADDR_14_15 GPIOA
#define GPION_GBA_ADDR_14    GPIO0
#define GPION_GBA_ADDR_15    GPIO1
#define GPIOP_GBA_ADDR_16_24 GPIOB

#define GPIOP_LED  GPIOA
#define GPION_LED  GPIO5

#define GPIOP_USART GPIOA
#define GPION_USART_TX GPIO2
#define GPION_USART_RX GPIO3

#endif
