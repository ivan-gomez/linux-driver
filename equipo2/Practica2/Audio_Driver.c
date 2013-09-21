#include <alsa/asoundlib.h>

/*Alsa handling*/
static char *device = "default";
int err;
unsigned int i;
snd_pcm_t *handle;
snd_pcm_sframes_t frames;

/*File handling*/
FILE *pFile;
long lSize;
char *buffer;
size_t result;

/*Set the volume*/
void SetAlsaMasterVolume(long volume)
{
	long min, max;
	snd_mixer_t *handle;
	snd_mixer_selem_id_t *sid;
	const char *card = "default";
	const char *selem_name = "Master";

	snd_mixer_open(&handle, 0);
	snd_mixer_attach(handle, card);
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

/*Set the frequency*/
void setParams(int samp)
{
	err = snd_pcm_set_params(handle,
		SND_PCM_FORMAT_S16_LE,
		SND_PCM_ACCESS_RW_INTERLEAVED,
		2,
		samp,
		1,
		500000);
	if ((err) < 0) { /* 0.5sec */
			printf("Playback open error: %s\n", snd_strerror(err));
			exit(EXIT_FAILURE);
	}
}

/*Play a file method*/
void play(int freq, char *file)
{
	err = snd_pcm_open(&handle, device, SND_PCM_STREAM_PLAYBACK, 0);
	if ((err) < 0) {
		printf("Playback open error: %s\n", snd_strerror(err));
		exit(EXIT_FAILURE);
	}
	setParams(freq);
	pFile = fopen(file , "rb");
	if (pFile == NULL) {
		fputs("File error", stderr);
		exit(1);
	}
	/*obtain file size*/
	fseek(pFile , 0 , SEEK_END);
	lSize = ftell(pFile);
	rewind(pFile);
	/*allocate memory to contain the whole file*/
	buffer = (char *) malloc(sizeof(char)*lSize);
	if (buffer == NULL) {
		fputs("Memory error", stderr);
		exit(2);
	}
	/*copy the file into the buffer:*/
	result = fread(buffer, 1, lSize, pFile);
	if (result != lSize) {
		fputs("Reading error", stderr);
		exit(3);
	}
	/*Playing*/
	printf("Playing\n");
	for (i = 0; i < 1; i++) {
		frames = snd_pcm_writei(handle, buffer, lSize/4);
		if (frames < 0)
			frames = snd_pcm_recover(handle, frames, 0);
		if (frames < 0) {
			printf("snd_pcm_writei failed: \
				%s\n", snd_strerror(err));
			break;
		}
		if (frames > 0 && frames < lSize/4)
			printf("Short write (expected %li, \
				wrote %li)\n", (long)lSize/4, frames);
	}
	printf("Done\n");
}

/*Record method*/
void record(int freq, char *file) \
{
	long int bytesToRecord = 524288;
	short buffer[bytesToRecord];

	err = snd_pcm_open(&handle, device, SND_PCM_STREAM_CAPTURE, 0);
	if ((err) < 0) {
		printf("Record open error: %s\n", snd_strerror(err));
		exit(EXIT_FAILURE);
	}
	setParams(freq);
	printf("Recording\n");
	for (i = 0; i < 1; i++) {
		err = snd_pcm_readi(handle, buffer, bytesToRecord/4);
		if ((err) != bytesToRecord/4) {
			fprintf(stderr, "read from audio \
				interface failed (%s)\n",
			snd_strerror(err));
			exit(1);
		}
	}
	pFile = fopen(file, "w+");
	if (pFile != NULL)
		fwrite(buffer, sizeof(short), bytesToRecord, pFile);
	fclose(pFile);
	printf("Done\n");
	snd_pcm_close(handle);
}

int main(int argc, char **argv)
{

	if (argc < 2 || argc != 5) {
		printf("Need to have this Format:\n");
		printf("./audioDriver <P | R> <volume> <frequency> <file>\n");
		return -1;
	}
	if (atoi(argv[2]) <= 0 || atoi(argv[2]) > 100) {
		printf("Volume must be between 1 - 100\n");
		return -1;
	}
	if (atoi(argv[3]) < 8000 || atoi(argv[3]) > 48000) {
		printf("Sampling rate must be between 8000 - 48000\n");
		return -1;
	}
	SetAlsaMasterVolume(atoi(argv[2]));
	if (strcmp(argv[1], "P") == 0)
		play(atoi(argv[3]), argv[4]);
	else if (strcmp(argv[1], "R") == 0)
		record(atoi(argv[3]), argv[4]);
	else {
		printf("Need to have this Format:\n");
		printf("./audioDriver <P | R> <volume> <frequency> <file>\n");
		return -1;
	}

	return 0;
}
