#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include "i2c.h"

enum bmp280_reg_addrs {
	BMP280_CALIB00 = 0x88,
	/*
	...
	BMP280_CALIB25 = 0xA1,
	*/
	BMP280_CHIP_ID = 0xD0,
	BMP280_RESET = 0xE0,
	BMP280_STATUS = 0xF3,
	BMP280_CTRL_MEAS,
	BMP280_CONFIG,
	BMP280_PRESS_MSB = 0xF7,
	BMP280_PRESS_LSB,
	BMP280_PRESS_XLSB,
	BMP280_TEMP_MSB,
	BMP280_TEMP_LSB,
	BMP280_TEMP_XLSB,
};

#define BMP280_MODE_SLEEP 0x00
#define BMP280_MODE_FORCED 0x01
#define BMP280_MODE_NORMAL 0x03

#define BMP280_CMD_RESET 0xB6

#define BMP280_CHAR_TO_16BIT(ptr, idx) ((ptr[idx + 1] << 8 | ptr[idx]))
#define BMP280_CHAR_TO_24BIT(ptr, idx) ((int32_t)(((ptr[idx] << 16) | (ptr[idx + 1] << 8) | ptr[idx + 2]) >> 4))

typedef int32_t BMP280_S32_t;

// Returns temperature in DegC, double precision. Output value of "51.23" equals 51.23 DegC.
// t_fine carries fine temperature
double bmp280_compensate_T_double(BMP280_S32_t adc_T, const uint16_t dig_T1, const int16_t dig_T2, const int16_t dig_T3, BMP280_S32_t *t_fine)
{
	double var1, var2, T;
	var1 = (((double)adc_T)/16384.0 - ((double)dig_T1)/1024.0) * ((double)dig_T2);
	var2 = ((((double)adc_T)/131072.0 - ((double)dig_T1)/8192.0) *
	(((double)adc_T)/131072.0 - ((double) dig_T1)/8192.0)) * ((double)dig_T3);
	*t_fine = (BMP280_S32_t)(var1 + var2);
	T = (var1 + var2) / 5120.0;
	return T;
}

// Returns pressure in Pa as double. Output value of "96386.2" equals 96386.2 Pa = 963.862 hPa
double bmp280_compensate_P_double(BMP280_S32_t adc_P,
	const uint16_t dig_P1, const int16_t dig_P2, const int16_t dig_P3,
	const int16_t dig_P4, const int16_t dig_P5, const int16_t dig_P6,
	const int16_t dig_P7, const int16_t dig_P8, const int16_t dig_P9,
	const BMP280_S32_t t_fine
)
{
	double var1, var2, p;
	var1 = ((double)t_fine/2.0) - 64000.0;
	var2 = var1 * var1 * ((double)dig_P6) / 32768.0;
	var2 = var2 + var1 * ((double)dig_P5) * 2.0;
	var2 = (var2/4.0)+(((double)dig_P4) * 65536.0);
	var1 = (((double)dig_P3) * var1 * var1 / 524288.0 + ((double)dig_P2) * var1) / 524288.0;
	var1 = (1.0 + var1 / 32768.0)*((double)dig_P1);
	if (var1 == 0.0)
	{
		return 0; // avoid exception caused by division by zero
	}
	p = 1048576.0 - (double)adc_P;
	p = (p - (var2 / 4096.0)) * 6250.0 / var1;
	var1 = ((double)dig_P9) * p * p / 2147483648.0;
	var2 = p * ((double)dig_P8) / 32768.0;
	p = p + (var1 + var2 + ((double)dig_P7)) / 16.0;
	return p;
}

static unsigned int bmp280_read(int file, enum bmp280_reg_addrs reg_addr, char *rdata, size_t rlen) {
	const char wdata[1] = { reg_addr };
	return i2c_rw(file, wdata, 1, rdata, rlen);
}

static unsigned int bmp280_write(int file, enum bmp280_reg_addrs reg_addr, const char *wdata, size_t wlen) {
	char *packet = calloc(wlen + 1, sizeof(char));
	packet[0] = reg_addr;
	memcpy(packet + 1, wdata, wlen);
	unsigned int result = i2c_rw(file, packet, wlen + 1, NULL, 0);
	free(packet);
	return result;
}

static int bmp280_get_callib(int file, char *dig) {
	if (bmp280_read(file, BMP280_CALIB00, dig, 26)) {
		return 1;
	}
	
	return 0;
}
/*
static int bmp280_print_part(int file) {
	char rdata[1];

	if (bmp280_read(file, BMP280_CHIP_ID, rdata, 1)) {
		return 1;
	}

	fputs("PART: ", stderr);

	if (rdata[0] == 0x58) {
		fputs("BMP280", stderr);
	}
	else {
		fputs("UNKNOWN", stderr);
	}

	fputs("\n", stderr);

	return 0;
}

static int bmp280_reset(int file) {
	const char wdata[1] = { BMP280_CMD_RESET };
	
	if (bmp280_write(file, BMP280_RESET, wdata, 1)) {
		return 1;
	}
	
	return 0;
}
*/

static int bmp280_get_status(int file, char *measuring, char *im_update) {
	char rdata;

	if (bmp280_read(file, BMP280_STATUS, &rdata, 1)) {
		return 1;
	}
	
	*measuring = (rdata & 0x08) >> 3;
	*im_update = rdata & 0x01;
	
	return 0;
}

static int bmp280_get_control(int file, char *mode, char *osrs_p, char *osrs_t) {
	char rdata;
	
	if (bmp280_read(file, BMP280_CTRL_MEAS, &rdata, 1)) {
		return 1;
	}
	
	*mode = rdata & 0x03;
	*osrs_p = (rdata >> 2) & 0x07;
	*osrs_t = (rdata >> 5) & 0x07;
	
	return 0;
}

static int bmp280_set_control(int file, const char mode, const char osrs_p, const char osrs_t) {
	char wdata = ((osrs_t & 0x07) << 5) | ((osrs_p & 0x07) << 2) | (mode & 0x03);
	
	if (bmp280_write(file, BMP280_CTRL_MEAS, &wdata, 1)) {
		return 1;
	}
	
	return 0;
}

static int bmp280_print_state(int file) {
	char measuring, im_update;
	
	if (bmp280_get_status(file, &measuring, &im_update)) {
		return 1;
	}
	
	fprintf(stderr, "measuring = %u, im_update = %u\n", measuring, im_update);
	
	char mode, osrs_p, osrs_t;
	
	if (bmp280_get_control(file, &mode, &osrs_p, &osrs_t)) {
		return 1;
	}
	
	fprintf(stderr, "mode = %u, osrs_p = %u, osrs_t = %u\n", mode, osrs_p, osrs_t);
	
	return 0;
}

static int bmp280_get_readings(int file, int32_t *press, int32_t *temp) {
	char rdata[6];
	
	if (bmp280_read(file, BMP280_PRESS_MSB, rdata, 6)) {
		return 1;
	}
	
	*press = BMP280_CHAR_TO_24BIT(rdata, 0);
	*temp = BMP280_CHAR_TO_24BIT(rdata, 3);
	
	return 0;
}
/*
static void hexdump(char *data, int len) {
	int i;

	for (i = 0; i < len; ++i) {
		printf("%02x ", data[i]);
	}

	puts("");
}
*/
static int bmp280_get_env(int file, char *buf) {
	/*
	if (bmp280_reset(file)) {
		return 1;
	}
	
	sleep(1);
	*/
	char dig[26];
	
	if (bmp280_get_callib(file, dig)) {
		return 1;
	}
	
	uint16_t dig_T1 = BMP280_CHAR_TO_16BIT(dig, 0);
	int16_t dig_T2 = BMP280_CHAR_TO_16BIT(dig, 2);
	int16_t dig_T3 = BMP280_CHAR_TO_16BIT(dig, 4);

	uint16_t dig_P1 = BMP280_CHAR_TO_16BIT(dig, 6);
	int16_t dig_P2 = BMP280_CHAR_TO_16BIT(dig, 8);
	int16_t dig_P3 = BMP280_CHAR_TO_16BIT(dig, 10);
	int16_t dig_P4 = BMP280_CHAR_TO_16BIT(dig, 12);
	int16_t dig_P5 = BMP280_CHAR_TO_16BIT(dig, 14);
	int16_t dig_P6 = BMP280_CHAR_TO_16BIT(dig, 16);
	int16_t dig_P7 = BMP280_CHAR_TO_16BIT(dig, 18);
	int16_t dig_P8 = BMP280_CHAR_TO_16BIT(dig, 20);
	int16_t dig_P9 = BMP280_CHAR_TO_16BIT(dig, 22);
	//int16_t dig_reserved = BMP280_CHAR_TO_16BIT(dig, 24);

	if (bmp280_print_state(file)) {
		return 1;
	}
	
	if (bmp280_set_control(file, 1, 1, 1)) {
		return 1;
	}

	int32_t adc_P, adc_T;
	
	if (bmp280_get_readings(file, &adc_P, &adc_T)) {
		return 1;
	}
	
	fprintf(stderr, "adc_P = %u, adc_T = %u\n", adc_P, adc_T);
	fprintf(stderr, "adc_P = %08x, adc_T = %08x\n", adc_P, adc_T);
	
	BMP280_S32_t t_fine;
	double T = bmp280_compensate_T_double(adc_T, dig_T1, dig_T2, dig_T3, &t_fine);	
	
	fprintf(stderr, "T = %.4f deg. C\n", T);
	
	double P = bmp280_compensate_P_double(adc_P, dig_P1, dig_P2, dig_P3, dig_P4, dig_P5, dig_P6, dig_P7, dig_P8, dig_P9, t_fine);
	
	fprintf(stderr, "P = %.4f hPa\n", P / 100);
	
	char fields[64];
	
	sprintf(fields, "&field1=%.4f", T);
	strcat(buf, fields);
	
	sprintf(fields, "&field2=%.4f", P / 100);
	strcat(buf, fields);

	return 0;
}

void bmp280_write_data(char *buf, unsigned char i2c_id) {
	int file = i2c_connect(i2c_id, 0x77);

	if (file == -1) {
		return;
	}

	//bmp280_print_part(file);
	bmp280_get_env(file, buf);
	close(file);
}
