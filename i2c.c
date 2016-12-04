#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

int i2c_connect(unsigned char i2c_id, unsigned char addr) {
	char filename[14];
	strcpy(filename, "/dev/i2c-");

	char id[5];
	sprintf(id, "%u", i2c_id);

	strcat(filename, id);
	fputs(filename, stderr);
	fputs("\n", stderr);

	int file;

	if ((file = open(filename, O_RDWR)) < 0) {
		perror("open");
		return -1;
	}

	if (ioctl(file, I2C_SLAVE, addr) < 0) {
		perror("ioctl I2C_SLAVE");
		fprintf(stderr, "Kernel module is using the device - force-attaching!\n");

		if (ioctl(file, I2C_SLAVE_FORCE, addr) < 0) {
			perror("ioctl I2C_SLAVE_FORCE");
			close(file);
			return -1;
		}
	}

	return file;
}

unsigned int i2c_rw(int file, const char *wdata, size_t wlen, char *rdata, size_t rlen) {
	if (wlen) {
		if (write(file, wdata, wlen) != wlen) {
			perror("write");
			return 1;
		}
	}

	if (rlen) {
		int retries = 10;
		int result = read(file, rdata, rlen);

		while (
			result == -1 && // read error
			errno == ENXIO && // NACK
			(--retries > 0)
		) {
			result = read(file, rdata, rlen); // retry
			usleep(5000); // wait 5 ms
		}

		if (result != rlen) {
			perror("read");
			return 2;
		}
	}

	return 0;
}
