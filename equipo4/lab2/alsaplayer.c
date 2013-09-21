/*
* Player/Recorder & sound controller using ALSA controller
*
* Compile:
* $ cc -o play alsa_player.c -lasound
*
* Usage:
* $ ./play <P-R> <1-100 volumen> <sample_rate> <channels> <seconds> <file>
*
*/

#include <alsa/asoundlib.h>
#include <stdio.h>
#include "alsaplayer.h"

#define PCM_DEVICE "default"

int main(int argc, char **argv)
{
	unsigned int pcm, tmp;
	char action;
	int rate, channels, seconds, volume, fd;
	snd_pcm_t *pcm_handle;
	snd_pcm_hw_params_t *params;
	snd_pcm_uframes_t frames;
	snd_pcm_stream_t stream;
	char *buff;
	int buff_size, loops;

	if (argc != 7) {
		printf("Usage: %s <P-R> <1-100 volume> <sample_rate> <channels> <seconds> <file>\n",
			argv[0]);
		return -1;
	}

	/* Parameters decoding */
	action = *argv[1];
	volume = atoi(argv[2]);
	rate = atoi(argv[3]);
	channels = atoi(argv[4]);
	seconds = atoi(argv[5]);
	fd = get_fd_file(action, argv[6]);

	/* Parameters validation */
	if (get_errors_params_validation(action, volume, channels,
		rate, seconds, fd) > 0)
		return -1;

	/* Allocate params object & configure pcm*/
	snd_pcm_hw_params_alloca(&params);
	configure_pcm(&pcm_handle, params, action, channels, rate, volume);

	/* Print summary */
	print_summary_params(action, volume, channels, rate, seconds);

	/* Get period size */
	snd_pcm_hw_params_get_period_size(params, &frames, 0);

	/* Allocate buffer */
	buff_size = frames * channels * 2 /* 2 -> sample size */;
	buff = (char *) malloc(buff_size);

	/* Get period time */
	snd_pcm_hw_params_get_period_time(params, &tmp, NULL);

	for (loops = (seconds * 1000000) / tmp; loops > 0; loops--) {
		/* Playback */
		if (action == 'P') {
			/* Read buffer from file */
			pcm = read(fd, buff, buff_size);
			if (pcm == 0) {
				printf("ERROR: Select a smaller time.\n");
				return 0;
			}

			/* Write into audio driver */
			pcm = snd_pcm_writei(pcm_handle, buff, frames);
			if (pcm == -EPIPE) {
				printf("XRUN.\n");
				snd_pcm_prepare(pcm_handle);
			} else if (pcm < 0)
				printf("ERROR. Can't write PCM device. %s\n",
					snd_strerror(pcm));
		} else {
		/* Record */
			/* Read buffer from audio driver */
			pcm = snd_pcm_readi(pcm_handle, buff, frames);
			if (pcm == -EPIPE) {
				printf("XRUN.\n");
				snd_pcm_prepare(pcm_handle);
			} else if (pcm < 0)
				printf("ERROR. Can't write PCM device. %s\n",
					snd_strerror(pcm));
			/* Write buffer info file */
			pcm = write(fd, buff, buff_size);
			if (pcm == 0) {
				printf("ERROR: Select a smaller time.\n");
				return 0;
			}
		}
	}
	/* Release objects */
	close(fd);
	snd_pcm_drain(pcm_handle);
	snd_pcm_close(pcm_handle);
	free(buff);
	return 0;
}

/*
  * Configure PCM acordingly to parameters
  * NAME: configure_pcm
  * PARAMETERS:
  *  snd_pcm_t **pcm_handle - double pointe to pcm object
  *  snd_pcm_hw_params_t *params - pointer to params object
  *  char action - P(playback) or R(record)
  *  int channels - 1 or 2
  *  int rate - positive integer of ratio
  *  int volume - volume 1-100
*/
void configure_pcm(snd_pcm_t **pcm_handle, snd_pcm_hw_params_t *params,
	char action, int channels, int rate, int volume)
{
	int pcm = 0;
	/* Open the PCM device in playback mode */
	pcm = snd_pcm_open(pcm_handle, PCM_DEVICE, get_pcm_stream(action), 0);
	if (pcm < 0)
		printf("ERROR: Can't open \"%s\" PCM device. %s\n",
					PCM_DEVICE, snd_strerror(pcm));
	/* Fill parameters object with default values*/
	snd_pcm_hw_params_any(*pcm_handle, params);

	/* Set parameters - 16 bits, little endian */
	pcm = snd_pcm_hw_params_set_access(*pcm_handle, params,
		SND_PCM_ACCESS_RW_INTERLEAVED);
	if (pcm < 0)
		printf("ERROR: Can't set interleaved mode. %s\n",
			snd_strerror(pcm));

	/* Set pcm format */
	pcm = snd_pcm_hw_params_set_format(*pcm_handle, params,
		SND_PCM_FORMAT_S16_LE);
	if (pcm < 0)
		printf("ERROR: Can't set format. %s\n", snd_strerror(pcm));

	/* Set channel parameters */
	snd_pcm_hw_params_set_channels(*pcm_handle, params, channels);

	/* Set rate parameters */
	snd_pcm_hw_params_set_rate_near(*pcm_handle, params, &rate, 0);

	/* Write parameters */
	pcm = snd_pcm_hw_params(*pcm_handle, params);
	if (pcm < 0)
		printf("ERROR: Can't set harware parameters. %s\n",
			snd_strerror(pcm));
	/* Set volume */
	set_master_volume(volume);
}

/*
  * Print all parameters as information
  * NAME: print_summary_params
  * PARAMETERS:
  *  char action - P(playback) or R(record)
  *  int volume - volume 1-100
  *  int channels - 1 or 2
  *  int rate - positive integer of ratio
  *  int seconds - positive integer to play or record
*/
void print_summary_params(char action, int volume, int channels,
	int rate, int seconds)
{
	if (action == 'P')
		printf("Playing...\n");
	else
		printf("Recording...\n");
	printf("volume: %i\n", volume);
	printf("channels: %i\n", channels);
	printf("rate: %d bps\n", rate);
	printf("seconds: %d\n", seconds);
}

/*
  * Get file descriptor from file
  * NAME: get_fd_file
  * RETURNS: int -1 if unsuccessful else file descriptor
  * PARAMETERS:
  *  char action - P(playback) or R(record)
  *  char *file - string of filename
*/
int get_fd_file(char action, char *file)
{
	if (action == 'P')
		return open(file, O_RDONLY);
	else if (action == 'R')
		return open(file, O_RDWR | O_CREAT, 0666);
	else
		return -1;
}

/*
  * Get pcm stream depending on action type
  * NAME: get_pcm_stream
  * RETURNS: snd_pcm_stream_t - pcm stream for playback or record
  * PARAMETERS:
  *  char action - P(playback) or R(record)
*/
snd_pcm_stream_t get_pcm_stream(char action)
{
	if (action == 'P')
		return SND_PCM_STREAM_PLAYBACK;
	else
		return SND_PCM_STREAM_CAPTURE;
}

/*
  * NAME: get_errors_params_validation
  * RETURNS: snd_pcm_stream_t - pcm stream for playback or record
  * PARAMETERS:
  *  char action - P(playback) or R(record)
  *  int volume - volume 1-100
  *  int channels - 1 or 2
  *  int seconds - positive integer to play or record
  *  int rate - positive integer of ratio
  *  int fd_file - file descriptor or file
*/
int get_errors_params_validation(char action, int volume, int channels,
	int rate, int seconds, int fd_file)
{
	int errors = 0;
	if (action == 'P' && fd_file < 0) {
		printf("ERROR: File doesn't exists\n");
		errors++;
	}
	if (action != 'P' && action != 'R') {
		printf("ERROR: Action parameter must be P or R\n");
		errors++;
	}
	if (volume < 0 || volume > 100) {
		printf("ERROR: Volume parameter must be between 1-100\n");
		errors++;
	}
	if (action == 'P' && channels < 1 || channels > 2) {
		printf("ERROR: Channels must be between 1-2 for playback\n");
		errors++;
	}
	if (action == 'R' && channels != 1) {
		printf("ERROR: Channel must be mono for recording\n");
		errors++;
	}
	if (rate < 1) {
		printf("ERROR: Rate parameter must be positive\n");
		errors++;
	}
	if (seconds < 1) {
		printf("ERROR: Seconds parameter must be positive\n");
		errors++;
	}
	return errors;
}

/*
  * NAME: set_master_volume
  * PARAMETERS:
  *  long volume - volume 1-100
*/
void set_master_volume(long volume)
{
	long min, max;
	snd_mixer_t *handle;
	snd_mixer_selem_id_t *sid;
	    const char *selem_name = "Master";

	/* Open volume handle of mixer-pcm;*/
	snd_mixer_open(&handle, 0);
	snd_mixer_attach(handle, PCM_DEVICE);
	snd_mixer_selem_register(handle, NULL, NULL);
	snd_mixer_load(handle);

	snd_mixer_selem_id_alloca(&sid);
	snd_mixer_selem_id_set_index(sid, 0);
	snd_mixer_selem_id_set_name(sid, selem_name);
	snd_mixer_elem_t *elem = snd_mixer_find_selem(handle, sid);

	/* Set volume according to max range value */
	snd_mixer_selem_get_playback_volume_range(elem, &min, &max);
	snd_mixer_selem_set_playback_volume_all(elem, volume * max / 100);

	snd_mixer_close(handle);
}
