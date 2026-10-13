#pragma once
// Pin map for DS3231 software I2C
// Default: PA4=SCL, PA3=SDA (user wiring)
// Alternatives:
//  - PB6=SCL, PB7=SDA (I2C1 pins)
//  - PA6=SCL, PA7=SDA (user wiring)
//  - PB8=SCL, PB9=SDA (I2C1 remap)
//  - PB10=SCL, PB11=SDA (I2C2 pins)

#define DS3231_I2C_SCL_PORT GPIOA
#define DS3231_I2C_SCL_PIN GPIO_PIN_4
#define DS3231_I2C_SDA_PORT GPIOA
#define DS3231_I2C_SDA_PIN GPIO_PIN_3
