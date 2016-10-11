#include <verilated.h>
#include <verilated_vcd_c.h>
#include "Vtestbench.h"

#include <stdbool.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>

#include <netinet/in.h>
#include <netinet/ip6.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <net/route.h>

#include <linux/if.h>
#include <linux/if_tun.h>
#include <linux/if_ether.h>

#include <time.h>
#define BUF_MAX_ASCII    16000
#define BUF_MAX          9400


#define SFP_CLK               (64/2)        // 6.4 ns (156.25 MHz)

#define WAVE_FILE_NAME        "wave.vcd"
#define SIM_TIME_RESOLUTION   "100 ps"
#define SIM_TIME              10000000000       // 100 us

#define __packed    __attribute__((__packed__))

#define rx_tdata    sim->s_axis_rx_tdata
#define rx_tkeep    sim->s_axis_rx_tkeep
#define rx_tlast    sim->s_axis_rx_tlast
#define rx_tvalid   sim->s_axis_rx_tvalid
#define tx_tdata    sim->m_axis_tx_tdata
#define tx_tkeep    sim->m_axis_tx_tkeep
#define tx_tlast    sim->m_axis_tx_tlast
#define tx_tvalid   sim->m_axis_tx_tvalid

#define AXIS_DWIDTH_BITS     64
#define AXIS_DWIDTH 	     AXIS_DWIDTH_BITS/8

static uint64_t t = 0;

struct pkt_buf {
	unsigned char buf[BUF_MAX_ASCII];
	uint32_t num;
	uint32_t len;
};

struct pkt_buf ibuf, obuf;

/*
 * tick: a tick
 */
static inline void tick(Vtestbench *sim, VerilatedVcdC *tfp)
{
	++t;
	sim->eval();
	tfp->dump(t);
}

/*
 * time_wait
 */
static inline void time_wait(Vtestbench *sim, VerilatedVcdC *tfp, uint32_t n)
{
	t += n;
	sim->eval();
	tfp->dump(t);
}

#ifdef zero
void pr_tdata(Vtestbench *sim)
{
	uint8_t *p;
	int i;

	if (result_tvalid) {
		printf("t=%u:", (uint32_t)t);
		p = (uint8_t *)&result_tdata;
		for (i = 0; i < 8; i++) {
			printf(" %02X", *(p++));
		}
		printf("\n");
	}
}

void pr_tlast(Vtestbench *sim)
{
	if (result_tlast) {
		printf("\n");
	}
}
#endif

/*
 * tap_init
 */
int tap_init(char *dev)
{
	struct ifreq ifr;
	int fd, err;

	fd = open("/dev/net/tun", O_RDWR);
	if (fd < 0) {
		perror("open");
		return -1;
	}

	memset(&ifr, 0, sizeof(ifr));
	ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
	if (*dev) {
		strncpy(ifr.ifr_name, dev, IFNAMSIZ);
	}

	err = ioctl(fd, TUNSETIFF, (void *)&ifr);
	if (err < 0) {
		perror("TUNSETIFF");
		close(fd);
		return err;
	}

	return fd;
}

int tap2axis(Vtestbench *sim, VerilatedVcdC *tfp)
{
	int i;
	uint8_t *p;
	int lcnt;
	int diff = ibuf.len - ibuf.num;
	p = (uint8_t *)&rx_tdata;
	
	if (diff <= 0)
		return 0;
	else if (diff <= AXIS_DWIDTH)
		rx_tlast = 1;

	rx_tdata = 0;
	rx_tvalid = 1;

	if (diff >= AXIS_DWIDTH) {
		for (i = 0; i < 8; i++) 
			*(p+7-i) = ibuf.buf[ibuf.num+i];
		rx_tkeep = 0xff;
	} else {
		for (i = 0; i < diff; i++) 
			*(p+7-i) = ibuf.buf[ibuf.num+i];
		rx_tkeep = ((0x0000ffff << diff) & 0xffff0000) >> 16;
	}

	ibuf.num += AXIS_DWIDTH;

	return 0;
}

uint8_t numofbits(uint8_t bits)
{
	uint8_t num = 0;
	uint8_t mask = 1;

	for (; mask != 0; mask = mask << 1)
		if (bits & mask) num++;

	return num;
}

int axis2tap(Vtestbench *sim, int tap_fd)
{
	int i, j, size, len, ret;
	uint8_t *p = (uint8_t *)&tx_tdata;

	if (tx_tvalid) {
		size = numofbits(tx_tkeep);
		obuf.len += size;
		for (i=0; i<size; i++)
			obuf.buf[obuf.num++] = *(p+7-i); 
	}

	if (tx_tvalid & tx_tlast) {
		ret = write(tap_fd, obuf.buf, obuf.len);
		obuf.num = 0;
		obuf.len = 0;
		return ret;
	}

	return 0;
}

/*
 * main
 */
int main(int argc, char **argv)
{
	int ret, tap_fd, maxfd;
	int i;
	char *dev = "tap0";
	fd_set fdset;
	unsigned char buf[BUF_MAX_ASCII*2];
	struct timeval timeout = { 0, 0 };
	tap_fd = tap_init(dev);
	if (tap_fd < 0) {
		perror("tap_fd");
		return 1;
	}
	maxfd = tap_fd;

	Verilated::commandArgs(argc, argv);
	Verilated::traceEverOn(true);

	VerilatedVcdC *tfp = new VerilatedVcdC;
	tfp->spTrace()->set_time_resolution(SIM_TIME_RESOLUTION);
	Vtestbench *sim = new Vtestbench;
	sim->trace(tfp, 99);
	tfp->open(WAVE_FILE_NAME);

	sim->cold_reset = 1;

	while (!Verilated::gotFinish()) {
		if ((t % SFP_CLK) == 0) {
			sim->clk100 = !sim->clk100;
			if (sim->clk100) {
				rx_tlast = 0;
				rx_tvalid = 0;
				rx_tdata = 0;

				FD_ZERO(&fdset);
				FD_SET(tap_fd, &fdset);

				ret = select(maxfd + 1, &fdset, NULL, NULL, &timeout);
				if (ret < 0) {
					perror("select");
					return 1;
				}

				if (FD_ISSET(tap_fd, &fdset)) {
					ret = read(tap_fd, ibuf.buf, sizeof(ibuf.buf));
					if (ret < 0) {
						perror("pktout read");
					} /*else {
						for (i=0; i<ret;i++)
							printf("%02x ", ibuf.buf[i]);
						printf("\n");
						//printf("pktout read:success\n");
					}*/
					ibuf.len = ret;
					ibuf.num = 0;
				}
				tap2axis(sim, tfp);
				axis2tap(sim, tap_fd);
			}
		}
		sim->cold_reset = 0;

		if (t > SIM_TIME)
			break;

		tick(sim, tfp);
	}

	tfp->close();
	sim->final();

	return 0;
}

