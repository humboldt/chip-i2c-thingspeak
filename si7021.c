#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "i2c.h"

#define BYTES2SHORT(X) (X[0] << 8 | X[1])

enum si7021_cmd_id {
	SI7021_MEAS_HUM,
	SI7021_MEAS_TEMP,
	SI7021_READ_TEMP,
	SI7021_RESET,
	SI7021_WRITE_USER_REG,
	SI7021_READ_USER_REG,
	SI7021_WRITE_HEATER_REG,
	SI7021_READ_HEATER_REG,
	SI7021_READ_SN_1,
	SI7021_READ_SN_2,
	SI7021_READ_FWREV,
	SI7021_CMDS_COUNT
};

static const char **si7021_cmd;

static unsigned int si7021_send_cmd(int file, enum si7021_cmd_id cmd_id, size_t wlen, char *rdata, size_t rlen) {
	return i2c_rw(file, si7021_cmd[cmd_id], wlen, rdata, rlen);
}

static int si7021_print_part(int file) {
	char resp[6];

	if (si7021_send_cmd(file, SI7021_READ_SN_2, 2, resp, 1)) {
		return 1;
	}

	fputs("PART: ", stderr);

	switch (resp[0]) {
		case 0x00:
		case 0xFF:
			fputs("Engineering sample", stderr);
		break;
		case 0x0D:
			fputs("Si7013", stderr);
		break;
		case 0x14:
			fputs("Si7020", stderr);
		break;
		case 0x15:
			fputs("Si7021", stderr);
		break;
		default:
			fputs("UNKNOWN", stderr);
		break;
	}

	fputs("\n", stderr);

	return 0;
}

static int si7021_print_sn(int file) {
	char resp[8];

	if (si7021_send_cmd(file, SI7021_READ_SN_1, 2, resp, 8)) {
		return 1;
	}

	fprintf(stderr, "SN: %02x%02x%02x%02x\n", resp[0], resp[2], resp[4], resp[6]);

	return 0;
}

static int si7021_print_fwrev(int file) {
	char resp;

	if (si7021_send_cmd(file, SI7021_READ_FWREV, 2, &resp, 1)) {
		return 1;
	}

	fputs("FWREV: ", stderr);

	switch (resp) {
		case 0xFF:
			fputs("1.0", stderr);
		break;
		case 0x20:
			fputs("2.0", stderr);
		break;
		default:
			fputs("UNKNOWN", stderr);
		break;
	}

	fputs("\n", stderr);

	return 0;
}

static int si7021_get_env(int file, char *buf) {
	char resp[3];

	if (si7021_send_cmd(file, SI7021_MEAS_HUM, 1, resp, 3)) {
		return 1;
	}

	float hum = 125.0 * BYTES2SHORT(resp) / 65536 - 6;

	if (si7021_send_cmd(file, SI7021_READ_TEMP, 1, resp, 2)) {
		return 1;
	}

	float temp = 175.72 * BYTES2SHORT(resp) / 65536 - 46.85;

	fprintf(stderr, "T: %f°C\nH: %f%%\n", temp, hum);

	char fields[64];
	sprintf(fields, "&field1=%f&field2=%f", temp, hum);
	strcat(buf, fields);

	return 0;
}

void si7021_write_data(char *buf, unsigned char i2c_id) {
	si7021_cmd = calloc(SI7021_CMDS_COUNT, sizeof(char *));

	si7021_cmd[SI7021_MEAS_HUM] = "\xF5";
	si7021_cmd[SI7021_MEAS_TEMP] = "\xF3";
	si7021_cmd[SI7021_READ_TEMP] = "\xE0";
	si7021_cmd[SI7021_RESET] = "\xFE";
	si7021_cmd[SI7021_WRITE_USER_REG] = "\xE6";
	si7021_cmd[SI7021_READ_USER_REG] = "\xE7";
	si7021_cmd[SI7021_WRITE_HEATER_REG] = "\x51";
	si7021_cmd[SI7021_READ_HEATER_REG] = "\x11";
	si7021_cmd[SI7021_READ_SN_1] = "\xFA\x0F";
	si7021_cmd[SI7021_READ_SN_2] = "\xFC\xC9";
	si7021_cmd[SI7021_READ_FWREV] = "\x84\xB8";

	int file = i2c_connect(i2c_id, 0x40);

	if (file == -1) {
		return;
	}

	si7021_print_part(file);
	si7021_print_sn(file);
	si7021_print_fwrev(file);
	si7021_get_env(file, buf);
	close(file);
}
