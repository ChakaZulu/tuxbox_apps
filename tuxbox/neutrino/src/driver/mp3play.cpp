/*
	Neutrino-GUI  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://dbox.cyberphoria.org/

	Kommentar:

	Diese GUI wurde von Grund auf neu programmiert und sollte nun vom
	Aufbau und auch den Ausbaumoeglichkeiten gut aussehen. Neutrino basiert
	auf der Client-Server Idee, diese GUI ist also von der direkten DBox-
	Steuerung getrennt. Diese wird dann von Daemons uebernommen.


	License: GPL

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "mp3play.h"

#include <mad.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <linux/soundcard.h>


FILE* soundfd;

static inline signed int scale(mad_fixed_t sample)
{
  /* round */
  sample += (1L << (MAD_F_FRACBITS - 16));

  /* clip */
  if (sample >= MAD_F_ONE)
    sample = MAD_F_ONE - 1;
  else if (sample < -MAD_F_ONE)
    sample = -MAD_F_ONE;

  /* quantize */
  return sample >> (MAD_F_FRACBITS + 1 - 16);
}

static enum mad_flow input(void *data,
		    struct mad_stream *stream)
{
  struct mad_buffer *buffer = (mad_buffer*) data;

  if (!buffer->length)
    return MAD_FLOW_STOP;

  mad_stream_buffer(stream, buffer->start, buffer->length);

  buffer->length = 0;

  return MAD_FLOW_CONTINUE;
}

static enum mad_flow output(void *data,
		     struct mad_header const *header,
		     struct mad_pcm *pcm)
{
  unsigned int nchannels, nsamples;
  mad_fixed_t const *left_ch, *right_ch;

  /* pcm->samplerate contains the sampling frequency */
printf("%i: output\n",__LINE__);

  nchannels = pcm->channels;
  nsamples  = pcm->length;
  left_ch   = pcm->samples[0];
  right_ch  = pcm->samples[1];

  while (nsamples--) {
    signed int sample;

    printf("%i: while samples\n",__LINE__);

    /* output sample(s) in 16-bit signed little-endian PCM */

    sample = scale(*left_ch++);
    fputc((sample >> 0) & 0xff,soundfd);
    fputc((sample >> 8) & 0xff,soundfd);

    if (nchannels == 2) {
	printf("%i: 2 channels\n",__LINE__);
      sample = scale(*right_ch++);
      fputc((sample >> 0) & 0xff,soundfd);
      fputc((sample >> 8) & 0xff,soundfd);
    }
  }
	return MAD_FLOW_CONTINUE;
}

static enum mad_flow error(void *data,
		    struct mad_stream *stream,
		    struct mad_frame *frame)
{
  mad_buffer *bdata = (mad_buffer*) data;
  printf("decoding error 0x%04x (%s) at byte offset %u\n",
	  stream->error, mad_stream_errorstr(stream),
	  stream->this_frame - bdata->start);
  return MAD_FLOW_CONTINUE; // MAD_FLOW_BREAK;
}

/*
 * NAME:	decode->input_read()
 * DESCRIPTION:	(re)fill decoder input buffer by reading a file descriptor
 */
/*
static
enum mad_flow decode_input_read(void *data, struct mad_stream *stream)
{
  struct player *player = data;
  struct input *input = &player->input;
  int len;

  if (input->eof)
    return MAD_FLOW_STOP;

  if (stream->next_frame) {
    memmove(input->data, stream->next_frame,
	    input->length = &input->data[input->length] - stream->next_frame);
  }

  do {
    len = read(input->fd, input->data + input->length,
	       MPEG_BUFSZ - input->length);
  }
  while (len == -1 && errno == EINTR);

  if (len == -1) {
    error("input", ":read");
    return MAD_FLOW_BREAK;
  }
  else if (len == 0) {
    input->eof = 1;

    assert(MPEG_BUFSZ - input->length >= MAD_BUFFER_GUARD);

    while (len < MAD_BUFFER_GUARD)
      input->data[input->length + len++] = 0;
  }

  mad_stream_buffer(stream, input->data, input->length += len);

  return MAD_FLOW_CONTINUE;
}
*/

void CMP3Player::play(){

	struct stat stat;
	unsigned char *fdm;
	struct mad_buffer buffer;
	struct mad_decoder decoder;
	int result, fd;

	
	fd=open("/neutrino.mp3", O_RDONLY);
	if (fd<0)
	{
		perror("open");
		return;
	}	

	fdm = (unsigned char*) mmap(0, stat.st_size, PROT_READ, MAP_SHARED, fd, 0);
	if (fdm == MAP_FAILED)
	{
		printf("mmap failed!\n");
		perror("mmap");
		return;
	}

	soundfd = fopen("/dev/sound/dsp", "w");
	::ioctl(fileno(soundfd), SNDCTL_DSP_SPEED, 44100);
	::ioctl(fileno(soundfd), SNDCTL_DSP_CHANNELS, 2);
	::ioctl(fileno(soundfd), SNDCTL_DSP_SETFMT, AFMT_S16_BE);

	mad_decoder_init(&decoder, &buffer,
		   input, 0 /* header */, 0 /* filter */, output,
		   error, 0 /* message */);

	buffer.start  = fdm;
	buffer.length = 4000000; //stat.st_size;

	printf("running mp3-decoder: %ul\n", stat.st_size);
	result = mad_decoder_run(&decoder, MAD_DECODER_MODE_SYNC);

	mad_decoder_finish(&decoder);


	if (munmap(fdm, stat.st_size) == -1)
	{
		printf("munmap failed!\n");
		return;
	}

	close(fd);
	::ioctl(fileno(soundfd), SNDCTL_DSP_RESET);
	fclose(soundfd);
}

