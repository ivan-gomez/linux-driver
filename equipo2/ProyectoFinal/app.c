#include <fcntl.h>
#include <alsa/asoundlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/ioctl.h>

#define IOC_MAGIC   0x94
/* Defines our ioctl call. */
#define PARA_IOC_SET_WRITE	_IO(IOC_MAGIC, 0x1)
#define PARA_IOC_SET_READ	_IO(IOC_MAGIC, 0x2)
#define PARA_IOC_SET_STROBE	_IO(IOC_MAGIC, 0x3)
#define PARA_IOC_CLEAR_STROBE	_IO(IOC_MAGIC, 0x4)

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
unsigned int volume;
char mute;
unsigned int freq;
int f;

/*Function Name: setAlsaMasterVolume
Parameters: volume(0 - 100)
Description: Set the volume*/
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

/*Function Name: setParams
Parameters: sampling rate
Description: Set the sampling rate*/
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

/*Function Name: play
Parameters: frequency(8000 - 48000) and the file
Description: Play the file*/
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
	printf("Playing\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
	for (i = 0; i < 1; i++) {
		frames = snd_pcm_writei(handle, buffer, lSize/4);
		if (frames < 0)
			frames = snd_pcm_recover(handle, frames, 0);
		if (frames < 0) {
			printf("snd_pcm_writei failed: %s\n", snd_strerror(err));
			break;
		}
		if (frames > 0 && frames < lSize/4)
			printf("Short write (expected %li, wrote %li)\n", (long)lSize/4, frames);
	}
	printf("Done\n");
}

/*Function Name: record
Parameters: frequency(8000 - 48000) and the file
Description: Record a file*/
void record(int freq, char *file)
{
	long int bytesToRecord = 524288;
	short buffer[bytesToRecord];

	err = snd_pcm_open(&handle, device, SND_PCM_STREAM_CAPTURE, 0);
	if ((err) < 0) {
		printf("Record open error: %s\n", snd_strerror(err));
		exit(EXIT_FAILURE);
	}
	setParams(freq);
	printf("Recording\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
	for (i = 0; i < 1; i++) {
		err = snd_pcm_readi(handle, buffer, bytesToRecord/4);
		if ((err) != bytesToRecord/4) {
			fprintf(stderr, "read from audio interface failed (%s)\n",
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

/*Function Name: menuApp
Parameters: menu level
Description: Display the menu in the terminal*/
void menuApp(int dato)
{
	switch (dato) {
	case 1:
		if (mute == 0)
			printf("[x] Play\n[ ] Record\n[ ] Set Volume\n[ ] Set Frequency\n[ ] Quit\n\n\nVolume: %i\nFrequency: %i KHz\n\n\n\n\n\n\n\n\n\n\n\n\n\n", (volume*10), (freq*8));
		else
			printf("[x] Play\n[ ] Record\n[ ] Set Volume\n[ ] Set Frequency\n[ ] Quit\n\n\nVolume: MUTE\nFrequency: %i KHz\n\n\n\n\n\n\n\n\n\n\n\n\n\n", (freq*8));
		break;
	case 2:
		if (mute == 0)
			printf("[ ] Play\n[x] Record\n[ ] Set Volume\n[ ] Set Frequency\n[ ] Quit\n\n\nVolume: %i\nFrequency: %i KHz\n\n\n\n\n\n\n\n\n\n\n\n\n\n", (volume*10), (freq*8));
		else
			printf("[ ] Play\n[x] Record\n[ ] Set Volume\n[ ] Set Frequency\n[ ] Quit\n\n\nVolume: MUTE\nFrequency: %i KHz\n\n\n\n\n\n\n\n\n\n\n\n\n\n", (freq*8));
		break;
	case 3:
		if (mute == 0)
			printf("[ ] Play\n[ ] Record\n[x] Set Volume\n[ ] Set Frequency\n[ ] Quit\n\n\nVolume: %i\nFrequency: %i KHz\n\n\n\n\n\n\n\n\n\n\n\n\n\n", (volume*10), (freq*8));
		else
			printf("[ ] Play\n[ ] Record\n[x] Set Volume\n[ ] Set Frequency\n[ ] Quit\n\n\nVolume: MUTE\nFrequency: %i KHz\n\n\n\n\n\n\n\n\n\n\n\n\n\n", (freq*8));
		break;
	case 4:
		if (mute == 0)
			printf("[ ] Play\n[ ] Record\n[ ] Set Volume\n[x] Set Frequency\n[ ] Quit\n\n\nVolume: %i\nFrequency: %i KHz\n\n\n\n\n\n\n\n\n\n\n\n\n\n", (volume*10), (freq*8));
		else
			printf("[ ] Play\n[ ] Record\n[ ] Set Volume\n[x] Set Frequency\n[ ] Quit\n\n\nVolume: MUTE\nFrequency: %i KHz\n\n\n\n\n\n\n\n\n\n\n\n\n\n", (freq*8));
		break;
	case 5:
		if (mute == 0)
			printf("[ ] Play\n[ ] Record\n[ ] Set Volume\n[ ] Set Frequency\n[x] Quit\n\n\nVolume: %i\nFrequency: %i KHz\n\n\n\n\n\n\n\n\n\n\n\n\n\n", (volume*10), (freq*8));
		else
			printf("[ ] Play\n[ ] Record\n[ ] Set Volume\n[ ] Set Frequency\n[x] Quit\n\n\nVolume: MUTE\nFrequency: %i KHz\n\n\n\n\n\n\n\n\n\n\n\n\n\n", (freq*8));
		break;
	case 10:
		printf("Set Volume\n<))) -[          ]+\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
		break;
	case 11:
		printf("Set Volume\n<))) -[|         ]+\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
		break;
	case 12:
		printf("Set Volume\n<))) -[||        ]+\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
		break;
	case 13:
		printf("Set Volume\n<))) -[|||       ]+\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
		break;
	case 14:
		printf("Set Volume\n<))) -[||||      ]+\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
		break;
	case 15:
		printf("Set Volume\n<))) -[|||||     ]+\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
		break;
	case 16:
		printf("Set Volume\n<))) -[||||||    ]+\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
		break;
	case 17:
		printf("Set Volume\n<))) -[|||||||   ]+\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
		break;
	case 18:
		printf("Set Volume\n<))) -[||||||||  ]+\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
		break;
	case 19:
		printf("Set Volume\n<))) -[||||||||| ]+\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
		break;
	case 20:
		printf("Set Volume\n<))) -[||||||||||]+\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
		break;
	case 21:
		printf("Set Volume\n<X   -[          ]+\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
		break;
	case 31:
		printf("Set Frequency\n[X] 8 KHz\n[ ] 16 KHz\n[ ] 24 KHz\n[ ] 32 KHz\n[ ] 40 KHz\n[ ] 48 KHz\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
		break;
	case 32:
		printf("Set Frequency\n[ ] 8 KHz\n[X] 16 KHz\n[ ] 24 KHz\n[ ] 32 KHz\n[ ] 40 KHz\n[ ] 48 KHz\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
		break;
	case 33:
		printf("Set Frequency\n[ ] 8 KHz\n[ ] 16 KHz\n[X] 24 KHz\n[ ] 32 KHz\n[ ] 40 KHz\n[ ] 48 KHz\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
		break;
	case 34:
		printf("Set Frequency\n[ ] 8 KHz\n[ ] 16 KHz\n[ ] 24 KHz\n[X] 32 KHz\n[ ] 40 KHz\n[ ] 48 KHz\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
		break;
	case 35:
		printf("Set Frequency\n[ ] 8 KHz\n[ ] 16 KHz\n[ ] 24 KHz\n[ ] 32 KHz\n[X] 40 KHz\n[ ] 48 KHz\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
		break;
	case 36:
		printf("Set Frequency\n[ ] 8 KHz\n[ ] 16 KHz\n[ ] 24 KHz\n[ ] 32 KHz\n[ ] 40 KHz\n[X] 48 KHz\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
		break;
	}
}

/*Function Name: setVolume
Description: Called to navigate in the volume menu*/
void setVolume()
{
	char flag = 1;
	ssize_t bytes;
	char rbuffer[4];
	while (flag) {
		if (mute == 0)
			menuApp(volume + 10);
		else
			menuApp(21);
		bytes = read(f, rbuffer, sizeof(rbuffer));
		if (rbuffer[1] >= 4)
			mute = 1;
		else
			mute = 0;
		if (rbuffer[0] == 2) {
			if (volume > 0)
				volume--;
		} else if (rbuffer[0] == 1) {
			if (volume < 10)
				volume++;
		} else if (rbuffer[1]&0x01 == 1) {
			if (mute == 0)
				SetAlsaMasterVolume(volume*10);
			else
				SetAlsaMasterVolume(0);
			flag = 0;
			menuApp(3);
		}
	}
}

/*Function Name: setFrequency
Description: Called to navigate in the frequency menu*/
void setFrequency()
{
	char flag = 1;
	ssize_t bytes;
	char rbuffer[4];
	while (flag) {
		menuApp(freq + 30);
		bytes = read(f, rbuffer, sizeof(rbuffer));
		if (rbuffer[0] == 2) {
			if (freq > 1)
				freq--;
		} else if (rbuffer[0] == 1) {
			if (freq < 6)
				freq++;
		} else if (rbuffer[1]&0x01 == 1) {
			flag = 0;
			menuApp(4);
		}
	}
}

/*Function Name: main
Description: Main function*/
int main(int argc, char **argv)
{
	char rbuffer[512];
	int menu;
	int ret;
	ssize_t bytes;
	char openApp = 0;

	menu = 1;
	volume = 10;
	mute = 0;
	freq = 4;
	f = open("/dev/control0", O_RDWR, 0666);
	if (f == -1) {
		printf("failed to open control0.\n");
		return 1;
	}
	while (openApp == 0) {
		printf("Presiona Enter para continuar\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
		bytes = read(f, rbuffer, sizeof(rbuffer));
		if (rbuffer[1]&0x01 == 1)
			openApp = 1;
	}
	while (openApp) {
		menuApp(menu);
		bytes = read(f, rbuffer, sizeof(rbuffer));
		if (rbuffer[0] == 2) {
			if (menu > 1)
				menu--;
		} else if (rbuffer[0] == 1) {
			if (menu < 5)
				menu++;
		} else if (rbuffer[1]&0x01 == 1) {
			switch (menu) {
			case 1:
				if (mute == 1)
					SetAlsaMasterVolume(0);
				else
					SetAlsaMasterVolume(volume*10);
					play((freq*8000), "ex");
				break;
			case 2:
				if (mute == 1)
					SetAlsaMasterVolume(0);
				else
					SetAlsaMasterVolume(volume*10);
					record((freq*8000), "ex");
				break;
			case 3:
				setVolume();
				break;
			case 4:
				setFrequency();
				break;
			case 5:
				openApp = 0;
				printf("Thank you, Good bye MOFO\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
				usleep(1000000);
				break;
			}
		}
		if (rbuffer[1] >= 4)
			mute = 1;
		else
			mute = 0;
	}
	close(f);
}
