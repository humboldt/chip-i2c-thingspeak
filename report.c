#include <stdio.h>
#include <string.h>

#include "axp209.h"
#include "si7021.h"

#include "config.h"

int main(int argc, char **argv) {
	char buf_axp209[2048];
	strcpy(buf_axp209, "http://api.thingspeak.com/update?api_key=" API_KEY_AXP209);
	axp209_write_data(buf_axp209, 0);
	puts(buf_axp209);

	char buf_si7021[2048];
	strcpy(buf_si7021, "http://api.thingspeak.com/update?api_key=" API_KEY_SI7021);
	si7021_write_data(buf_si7021, 1);
	puts(buf_si7021);

	return 0;
}
