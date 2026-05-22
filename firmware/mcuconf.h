#pragma once
#include_next <mcuconf.h>

#undef RP_I2C_USE_I2C1
#define RP_I2C_USE_I2C1 TRUE
#define RP_I2C_I2C1_SDA_PIN GP26
#define RP_I2C_I2C1_SCL_PIN GP27
