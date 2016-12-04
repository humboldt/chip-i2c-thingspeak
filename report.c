#include <stdio.h>
#include <string.h>

#include "config.h"

#if defined(API_KEY_AXP209) && defined(I2C_PORT_AXP209)
#include "axp209.h"
#endif

#if defined(API_KEY_SI7021) && defined(I2C_PORT_SI7021)
#include "si7021.h"
#endif

#if defined(API_KEY_BMP280) && defined(I2C_PORT_BMP280)
#include "bmp280.h"
#endif

int main(int argc, char **argv) {
	char buffer[2048];

#if defined(API_KEY_AXP209) && defined(I2C_PORT_AXP209)
	strcpy(buffer, "http://api.thingspeak.com/update?api_key=" API_KEY_AXP209);
	axp209_write_data(buffer, I2C_PORT_AXP209);
	puts(buffer);
#endif

#if defined(API_KEY_SI7021) && defined(I2C_PORT_SI7021)
	strcpy(buffer, "http://api.thingspeak.com/update?api_key=" API_KEY_SI7021);
	si7021_write_data(buffer, I2C_PORT_SI7021);
	puts(buffer);
#endif

#if defined(API_KEY_BMP280) && defined(I2C_PORT_BMP280)
	strcpy(buffer, "http://api.thingspeak.com/update?api_key=" API_KEY_BMP280);
	bmp280_write_data(buffer, I2C_PORT_BMP280);
	puts(buffer);
#endif

	return 0;
}
