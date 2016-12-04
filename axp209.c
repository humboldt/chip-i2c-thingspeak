#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "i2c.h"

enum axp209_reg_id {
	AXP209_VBUS_VOLTAGE,
	AXP209_VBUS_CURRENT,
	AXP209_TEMPERATURE,
	AXP209_BATTERY_VOLTAGE,
	AXP209_CHARGE_CURRENT,
	AXP209_DISCHARGE_CURRENT,
	AXP209_IPSOUT_VOLTAGE,
	AXP209_REGS_COUNT
};

static const char **axp209_reg;

static short axp209_get_value(int file, enum axp209_reg_id reg_id, unsigned char bits) {
	char buffer;
	short result;
	const char *request = axp209_reg[reg_id];

	if (i2c_rw(file, request, 1, &buffer, 1)) {
		return 0xffff;
	}

	result = buffer << bits;

	if (i2c_rw(file, request + 1, 1, &buffer, 1)) {
		return 0xffff;
	}

	result |= buffer;

	//fprintf(stderr, "READ: 0x%04x = %d\n", result, result);

	return result;
}

static int axp209_get_value_scaled(int file, enum axp209_reg_id reg_id, unsigned char bits, float step, float *result) {
	short value = axp209_get_value(file, reg_id, bits);

	if (value == 0xffff) {
		return 1;
	}

	*result = step * value / 1000;

	return 0;
}

static int axp209_get_vbus_voltage(int file, float *result) {
	return axp209_get_value_scaled(file, AXP209_VBUS_VOLTAGE, 4, 1.7, result);
}

static int axp209_get_vbus_current(int file, float *result) {
	return axp209_get_value_scaled(file, AXP209_VBUS_CURRENT, 4, 0.375, result);
}

static int axp209_get_temperature(int file, float *result) {
	short value = axp209_get_value(file, AXP209_TEMPERATURE, 4);

	if (value == 0xffff) {
		return 1;
	}

	*result = 0.1 * value - 144.7;

	return 0;
}

static int axp209_get_battery_voltage(int file, float *result) {
	return axp209_get_value_scaled(file, AXP209_BATTERY_VOLTAGE, 4, 1.1, result);
}

static int axp209_get_charge_current(int file, float *result) {
	return axp209_get_value_scaled(file, AXP209_CHARGE_CURRENT, 4, 0.5, result);
}

static int axp209_get_discharge_current(int file, float *result) {
	return axp209_get_value_scaled(file, AXP209_DISCHARGE_CURRENT, 5, 0.5, result);
}

static int axp209_get_ipsout_voltage(int file, float *result) {
	return axp209_get_value_scaled(file, AXP209_IPSOUT_VOLTAGE, 4, 1.4, result);
}

static int axp209_get_data(int file, char *buf) {
	float value;

	if (axp209_get_vbus_voltage(file, &value) == 0) {
		fprintf(stderr, "Uvbus: %.4fV\n", value);
		char fields[64];
		sprintf(fields, "&field1=%.4f", value);
		strcat(buf, fields);
	}

	if (axp209_get_vbus_current(file, &value) == 0) {
		fprintf(stderr, "Ivbus: %.6fA\n", value);
		char fields[64];
		sprintf(fields, "&field2=%.6f", value);
		strcat(buf, fields);
	}

	if (axp209_get_temperature(file, &value) == 0) {
		fprintf(stderr, "Tchip: %.1f°C\n", value);
		char fields[64];
		sprintf(fields, "&field3=%.1f", value);
		strcat(buf, fields);
	}

	if (axp209_get_battery_voltage(file, &value) == 0) {
		fprintf(stderr, "Ubatt: %.4fV\n", value);
		char fields[64];
		sprintf(fields, "&field4=%.4f", value);
		strcat(buf, fields);
	}

	if (axp209_get_charge_current(file, &value) == 0) {
		fprintf(stderr, "Icharge: %.4fA\n", value);
		char fields[64];
		sprintf(fields, "&field5=%.4f", value);
		strcat(buf, fields);
	}

	if (axp209_get_discharge_current(file, &value) == 0) {
		fprintf(stderr, "Idischarge: %.4fA\n", value);
		char fields[64];
		sprintf(fields, "&field6=%.4f", value);
		strcat(buf, fields);
	}

	if (axp209_get_ipsout_voltage(file, &value) == 0) {
		fprintf(stderr, "Uipsout: %.4fV\n", value);
		char fields[64];
		sprintf(fields, "&field7=%.4f", value);
		strcat(buf, fields);
	}

	return 0;
}

void axp209_write_data(char *buf, unsigned char i2c_id) {
	axp209_reg = calloc(AXP209_REGS_COUNT, sizeof(char *));

	axp209_reg[AXP209_VBUS_VOLTAGE] = "\x5A\x5B";
	axp209_reg[AXP209_VBUS_CURRENT] = "\x5C\x5D";
	axp209_reg[AXP209_TEMPERATURE] = "\x5E\x5F";
	axp209_reg[AXP209_BATTERY_VOLTAGE] = "\x78\x79";
	axp209_reg[AXP209_CHARGE_CURRENT] = "\x7A\x7B";
	axp209_reg[AXP209_DISCHARGE_CURRENT] = "\x7C\x7D";
	axp209_reg[AXP209_IPSOUT_VOLTAGE] = "\x7E\x7F";

	int file = i2c_connect(i2c_id, 0x34);

	if (file == -1) {
		return;
	}

	axp209_get_data(file, buf);
	close(file);
}
