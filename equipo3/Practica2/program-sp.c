#include "../alsa/asoundlib.h"
#include <errno.h>

/*Alsa handling*/
int err;
unsigned int i;
snd_pcm_t *handle;
snd_pcm_sframes_t frames;

/*File handling*/
FILE *pFile;
long lSize;
char *buffer;
size_t result;

/*Volume Configuration*/
void SetAlsaMasterVolume(long volume)
{
	long min, max;
	snd_mixer_t *handle;
	snd_mixer_selem_id_t *sid;
	const char *selem_name = "Master";

	snd_mixer_open(&handle, 0);
	snd_mixer_attach(handle, "default");
	snd_mixer_selem_register(handle, NULL, NULL);
	snd_mixer_load(handle);

	snd_mixer_selem_id_alloca(&sid);
	snd_mixer_selem_id_set_index(sid, 0);
	snd_mixer_selem_id_set_name(sid, selem_name);
	snd_mixer_elem_t *elem = snd_mixer_find_selem(handle, sid);

	snd_mixer_selem_get_playback_volume_range(elem, &min, &max);
	snd_mixer_selem_set_playback_volume_all(elem, volume * max / 100);

	snd_mixer_close(handle);
}

/*Sets parameters for PCM*/
void setParams(int samp)
{
	err = snd_pcm_set_params(handle,
		SND_PCM_FORMAT_S16_LE,
		SND_PCM_ACCESS_RW_INTERLEAVED,
		2,
		samp,
		1,
		500000);
	if (err < 0) { /* 0.5sec */
			printf("Playback open error: %s\n", snd_strerror(err));
			exit(EXIT_FAILURE);
	}
}

/*Function plays audio file*/
void play(int volume, int resample, char *file)
{
	pFile = fopen(file , "rb");
	if (pFile == NULL) {
		if (errno == ENOENT)
			printf("File doesn't exist\n");
		else
			printf("File error");

		exit(1);
	}

	printf("About to play the file: %s\n", file);
	printf("Volume: %i\n", volume);
	printf("Resample: %i\n", resample);

	err = snd_pcm_open(&handle, "default", SND_PCM_STREAM_PLAYBACK, 0);
	if (err < 0) {
		printf("Playback open error: %s\n", snd_strerror(err));
		exit(EXIT_FAILURE);
	}

	setParams(resample);

	/*Obtain file size*/
	fseek(pFile, 0, SEEK_END);
	lSize = ftell(pFile);
	rewind(pFile);

	/*Allocate memory to contain the whole file*/
	buffer = (char *) malloc(sizeof(char)*lSize);
	if (buffer == NULL) {
		printf("Memory error");
		exit(1);
	}

	/* copy the file into the buffer:*/
	result = fread(buffer, 1, lSize, pFile);
	if (result != lSize) {
		printf("Reading error");
		free(buffer);
		exit(1);
	}

	/*for (i = 0; i < 1; i++) {*/ /*For multiple channels or sound looping*/
		frames = snd_pcm_writei(handle, buffer, lSize/4);
		if (frames < 0)
			frames = snd_pcm_recover(handle, frames, 0);
		if (frames < 0) {
			printf("snd_pcm_writei failed: %s\n",
				snd_strerror(err));
			break;
		}
		if (frames > 0 && frames < lSize/4)
			printf("Short write (expected %li, wrote %li)\n",
				(long)lSize/4, frames);
	/*}*/

	fclose(pFile);
	free(buffer);

	snd_pcm_close(handle);
}

/*Function records sound*/
void record(int volume, int resample, char *file)
{
	long int bytesToRecord = 524288; /*(1024x1024/2)*/
	short buffer[bytesToRecord];

	printf("About to record: %s\n", file);
	printf("Volume: %i\n", volume);
	printf("Resample: %i\n", resample);
	err = snd_pcm_open(&handle, "default", SND_PCM_STREAM_CAPTURE, 0);
	if (err < 0) {
		printf("Record open error: %s\n", snd_strerror(err));
		exit(EXIT_FAILURE);
	}

	setParams(resample);

	printf("Recording...\n");
	/*for (i = 0; i < 1; i++) {*/ /*For multiple channels*/
	err = snd_pcm_readi(handle, buffer, bytesToRecord/4);
	if (err != bytesToRecord/4) {
		fprintf(stderr,
			"read from audio interface failed (%s)\n",
		snd_strerror(err));
		exit(1);
	}
	/*}*/
	printf("Recording done\n");

	/*Saves the buffer information to a file*/
	pFile = fopen(file, "w+");
	if (pFile != NULL) {
		fwrite(buffer, sizeof(short), bytesToRecord, pFile);
		fclose(pFile);
	}

	printf("File saved\n");

	snd_pcm_close(handle);
}

/*Message showing correct format to enter in terminal*/
void showUsage(void)
{
	printf("The format is the following:\n");
	printf("./playrecord <P | R> <volume> <sample_rate> <audio file>\n");
}

int main(int argc, char **argv)
{
	int volume, resample;

	if (argc != 5) {
		showUsage();
		return -1;
	}

	volume = atoi(argv[2]);
	resample = atoi(argv[3]);
	/*Volume control between 1 and 100*/
	if (volume > 0 && volume <= 100) {
		SetAlsaMasterVolume(volume);
	} else {
		printf("Ingresar valor entre 1 y \
100 para el volumen\n");
		return -1;
	}
	/*Frequency control between 8000 and 48000*/
	if (resample < 8000 || resample > 48000) {
		printf("Ingresar valor entre 8000 y \
48000 para el sampling rate\n");
		return -1;
	}

	/*Check if P or R is typed*/
	if (strcmp(argv[1], "P") == 0) {
		play(volume, resample, argv[4]);
	} else if (strcmp(argv[1], "R") == 0) {
		record(volume, resample, argv[4]/*, time*/);
	} else {
		showUsage();
		return -1;
	}

	return 0;
}
