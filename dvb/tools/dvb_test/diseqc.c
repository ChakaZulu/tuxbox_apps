/*
 * Test sending DiSEqC commands on a SAT frontend.
 *
 * usage: FRONTEND=/dev/dvb/adapterX/frontendX diseqc [test_seq_no]
 */

#include <pthread.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/dvb/frontend.h>


struct diseqc_cmd {
	struct dvb_diseqc_master_cmd cmd;
	uint32_t wait;
};


struct diseqc_sequence {
	int fd;
	fe_sec_voltage_t voltage;
	struct diseqc_cmd **cmd;
	fe_sec_mini_cmd_t burst;
	fe_sec_tone_mode_t tone;
};

/*--------------------------------------------------------------------------*/
static inline
void msleep(uint32_t msec)
{
	struct timespec req = { msec / 1000, 1000000 * (msec % 1000) };

	while (nanosleep(&req, &req))
		;
}


void diseqc_send_msg(int fd, fe_sec_voltage_t v, struct diseqc_cmd **cmd,
		     fe_sec_tone_mode_t t, fe_sec_mini_cmd_t b)
{
	ioctl(fd, FE_SET_TONE, SEC_TONE_OFF);
	ioctl(fd, FE_SET_VOLTAGE, v);

	msleep(15);
	while (*cmd) {
		printf("msg: %02x %02x %02x %02x %02x %02x\n",
		       (*cmd)->cmd.msg[0], (*cmd)->cmd.msg[1],
		       (*cmd)->cmd.msg[2], (*cmd)->cmd.msg[3],
		       (*cmd)->cmd.msg[4], (*cmd)->cmd.msg[5]);

		ioctl(fd, FE_DISEQC_SEND_MASTER_CMD, &(*cmd)->cmd);
		msleep((*cmd)->wait);
		cmd++;
	}

	printf("%s: ", __FUNCTION__);

	printf(" %s ", v == SEC_VOLTAGE_13 ? "SEC_VOLTAGE_13" :
	       v == SEC_VOLTAGE_18 ? "SEC_VOLTAGE_18" : "???");

	printf(" %s ", b == SEC_MINI_A ? "SEC_MINI_A" :
	       b == SEC_MINI_B ? "SEC_MINI_B" : "???");

	printf(" %s\n", t == SEC_TONE_ON ? "SEC_TONE_ON" :
	       t == SEC_TONE_OFF ? "SEC_TONE_OFF" : "???");

	msleep(15);
	ioctl(fd, FE_DISEQC_SEND_BURST, b);

	msleep(15);
	ioctl(fd, FE_SET_TONE, t);
}

/*--------------------------------------------------------------------------*/
#if 0

static
void *async_diseqc_thread(void *data)
{
	struct diseqc_sequence *s = data;
	struct diseqc_cmd **cmd = &s->cmd;

	pthread_cleanup_push(free, data);
	ioctl(s->fd, FE_SET_VOLTAGE, SEC_TONE_OFF);
	ioctl(s->fd, FE_SET_VOLTAGE, s->voltage);
	msleep(15);

	while (*cmd) {
		pthread_testcancel();
		ioctl(s->fd, FE_DISEQC_SEND_MASTER_CMD, &(*cmd)->cmd);
		msleep((*cmd)->wait);
		cmd++;
	}

	msleep(15);
	ioctl(s->fd, FE_DISEQC_SEND_BURST, s->burst);
	msleep(15);
	ioctl(s->fd, FE_SET_TONE, s->tone);
	pthread_cleanup_pop(1);
	return NULL;
}


void diseqc_send_msg_async(struct diseqc_sequence *seq, pthread_t * t)
{
	struct diseqc_sequence *s = malloc(sizeof(struct diseqc_sequence));

	memcpy(s, seq, sizeof(struct diseqc_sequence));

	if (t) {
		pthread_cancel(*t);
		pthread_join(*t, NULL);
	}

	pthread_create(t, NULL, async_diseqc_thread, s);
}
#endif
/*--------------------------------------------------------------------------*/

#if 0
struct diseqc_cmd test_cmds[] = {
	{{{0xe0, 0x10, 0x20, 0x00, 0x00, 0x00}, 3}, 100},	/* low LOF */
	{{{0xe0, 0x10, 0x21, 0x00, 0x00, 0x00}, 3}, 100},	/* vertical */
	{{{0xe0, 0x10, 0x22, 0x00, 0x00, 0x00}, 3}, 1000},	/* pos A */

	{{{0xe0, 0x10, 0x20, 0x00, 0x00, 0x00}, 3}, 100},	/* low LOF */
	{{{0xe0, 0x10, 0x25, 0x00, 0x00, 0x00}, 3}, 100},	/* horizontal */
	{{{0xe0, 0x10, 0x22, 0x00, 0x00, 0x00}, 3}, 1000},	/* pos A */

	{{{0xe0, 0x10, 0x24, 0x00, 0x00, 0x00}, 3}, 100},	/* high LOF */
	{{{0xe0, 0x10, 0x21, 0x00, 0x00, 0x00}, 3}, 100},	/* vertical */
	{{{0xe0, 0x10, 0x22, 0x00, 0x00, 0x00}, 3}, 1000},	/* pos A */

	{{{0xe0, 0x10, 0x24, 0x00, 0x00, 0x00}, 3}, 100},	/* high LOF */
	{{{0xe0, 0x10, 0x25, 0x00, 0x00, 0x00}, 3}, 100},	/* horizontal */
	{{{0xe0, 0x10, 0x22, 0x00, 0x00, 0x00}, 3}, 1000},	/* pos A */

	{{{0xe0, 0x10, 0x20, 0x00, 0x00, 0x00}, 3}, 100},	/* low LOF */
	{{{0xe0, 0x10, 0x21, 0x00, 0x00, 0x00}, 3}, 100},	/* vertical */
	{{{0xe0, 0x10, 0x26, 0x00, 0x00, 0x00}, 3}, 1000},	/* pos B */

	{{{0xe0, 0x10, 0x20, 0x00, 0x00, 0x00}, 3}, 100},	/* low LOF */
	{{{0xe0, 0x10, 0x25, 0x00, 0x00, 0x00}, 3}, 100},	/* horizontal */
	{{{0xe0, 0x10, 0x26, 0x00, 0x00, 0x00}, 3}, 1000},	/* pos B */

	{{{0xe0, 0x10, 0x24, 0x00, 0x00, 0x00}, 3}, 100},	/* high LOF */
	{{{0xe0, 0x10, 0x21, 0x00, 0x00, 0x00}, 3}, 100},	/* vertical */
	{{{0xe0, 0x10, 0x26, 0x00, 0x00, 0x00}, 3}, 1000},	/* pos B */

	{{{0xe0, 0x10, 0x24, 0x00, 0x00, 0x00}, 3}, 100},	/* high LOF */
	{{{0xe0, 0x10, 0x25, 0x00, 0x00, 0x00}, 3}, 100},	/* horizontal */
	{{{0xe0, 0x10, 0x26, 0x00, 0x00, 0x00}, 3}, 1000}	/* pos B */
};
#endif

struct diseqc_cmd test_cmds[] = {
	{{{0xe0, 0x10, 0x38, 0xf0, 0x00, 0x00}, 4}, 0},
	{{{0xe0, 0x10, 0x38, 0xf2, 0x00, 0x00}, 4}, 0},
	{{{0xe0, 0x10, 0x38, 0xf1, 0x00, 0x00}, 4}, 0},
	{{{0xe0, 0x10, 0x38, 0xf3, 0x00, 0x00}, 4}, 0},
	{{{0xe0, 0x10, 0x38, 0xf4, 0x00, 0x00}, 4}, 0},
	{{{0xe0, 0x10, 0x38, 0xf6, 0x00, 0x00}, 4}, 0},
	{{{0xe0, 0x10, 0x38, 0xf5, 0x00, 0x00}, 4}, 0},
	{{{0xe0, 0x10, 0x38, 0xf7, 0x00, 0x00}, 4}, 0},
	{{{0xe0, 0x10, 0x38, 0xf8, 0x00, 0x00}, 4}, 0},
	{{{0xe0, 0x10, 0x38, 0xfa, 0x00, 0x00}, 4}, 0},
	{{{0xe0, 0x10, 0x38, 0xf9, 0x00, 0x00}, 4}, 0},
	{{{0xe0, 0x10, 0x38, 0xfb, 0x00, 0x00}, 4}, 0},
	{{{0xe0, 0x10, 0x38, 0xfc, 0x00, 0x00}, 4}, 0},
	{{{0xe0, 0x10, 0x38, 0xfe, 0x00, 0x00}, 4}, 0},
	{{{0xe0, 0x10, 0x38, 0xfd, 0x00, 0x00}, 4}, 0},
	{{{0xe0, 0x10, 0x38, 0xff, 0x00, 0x00}, 4}, 0}
};



int main(int argc, char **argv)
{
	struct diseqc_cmd *cmd[2] = { NULL, NULL };
	char *fedev = "/dev/dvb/adapter0/frontend0";
	int fd;

	if (getenv("FRONTEND"))
		fedev = getenv("FRONTEND");

	printf("diseqc test: using '%s'\n", fedev);

	if ((fd = open(fedev, O_RDWR)) < 0) {
		perror("open");
		return -1;
	}

	if (argc > 1) {
		int i = atol(argv[1]);
		cmd[0] = &test_cmds[i];
		diseqc_send_msg(fd,
				i % 2 ? SEC_VOLTAGE_18 : SEC_VOLTAGE_13,
				cmd,
				(i/2) % 2 ? SEC_TONE_ON : SEC_TONE_OFF,
				(i/4) % 2 ? SEC_MINI_B : SEC_MINI_A);
	} else {
		unsigned int j;

		for (j=0; j<sizeof(test_cmds)/sizeof(struct diseqc_cmd); j++) {
			cmd[0] = &test_cmds[j];
			diseqc_send_msg(fd,
					j % 2 ? SEC_VOLTAGE_18 : SEC_VOLTAGE_13,
					cmd,
					(j/2) % 2 ? SEC_TONE_ON : SEC_TONE_OFF,
					(j/4) % 2 ? SEC_MINI_B : SEC_MINI_A);
			msleep (1000);
		}
	}

	close(fd);

	return 0;
}
