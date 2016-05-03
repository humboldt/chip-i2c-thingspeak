#ifndef _I2C_H_

int i2c_connect(unsigned char i2c_id, unsigned char addr);
unsigned int i2c_rw(int file, const char *wdata, size_t wlen, char *rdata, size_t rlen);

#endif
