#include "../alsa/asoundlib.h"

/*Variables Globales*/
static char *device = "default";
int err;
snd_pcm_t *handle;
snd_pcm_sframes_t frames;
FILE *pFile;
long lSize;
char *buffer;
size_t result;

void SetAlsaMasterVolume(char volumen)
{
	long int min, max;
	snd_mixer_t *handler;
	snd_mixer_selem_id_t *sid;
	const char *selem_name = "Master";

	snd_mixer_open(&handler, 0);
	snd_mixer_attach(handler, device);
	snd_mixer_selem_register(handler, NULL, NULL);
	snd_mixer_load(handler);

	snd_mixer_selem_id_alloca(&sid);
	snd_mixer_selem_id_set_index(sid, 0);
	snd_mixer_selem_id_set_name(sid, selem_name);
	snd_mixer_elem_t *elem = snd_mixer_find_selem(handler, sid);

	snd_mixer_selem_get_playback_volume_range(elem, &min, &max);
	snd_mixer_selem_set_playback_volume_all(elem, volumen * max / 100);

	snd_mixer_close(handler);
}

void reproducir(char volumen, int muestreo, char *archivo)
{
	printf("Reproduciendo %s\n", archivo);
	printf("Volumen: %d\n", volumen);
	printf("Muestreo: %i\n", muestreo);
	err = snd_pcm_open(&handle, device, SND_PCM_STREAM_PLAYBACK, 0);

	if (err < 0) {
		printf("Error abriendo el archivo: %s\n", snd_strerror(err));
		exit(EXIT_FAILURE);
	}
	err = snd_pcm_set_params(handle, SND_PCM_FORMAT_S16_LE,
		SND_PCM_ACCESS_RW_INTERLEAVED, 2, muestreo,
		1, 500000);
	if (err < 0) {
		printf("Error estableciendo los parametros: %s\n",
			snd_strerror(err));
		exit(EXIT_FAILURE);
	}

	pFile = fopen(archivo , "rb");
	if (pFile == NULL) {
		printf("No se pudo abrir el archivo");
		return;
	}

	/* obtener tamaño de archivo */
	fseek(pFile , 0 , SEEK_END);
	lSize = ftell(pFile);
	rewind(pFile);

	/* reservar memoria para el archivo completo*/
	buffer = (char *) malloc(sizeof(char)*lSize);
	if (buffer == NULL) {
		printf("Error abriendo el archivo");
		fclose(pFile);
		return;
	}

	/* copiar el archivo dentro del buffer */
	result = fread(buffer, 1, lSize, pFile);
	if (result != lSize) {
		printf("Fallo en la lectura");
		free(buffer);
		fclose(pFile);
		return;
	}

	frames = snd_pcm_writei(handle, buffer, lSize/4);
	if (frames < 0)
		frames = snd_pcm_recover(handle, frames, 0);
	if (frames < 0)
		printf("fallo snd_pcm_writei: %s\n", snd_strerror(err));
	if (frames > 0 && frames < lSize/4)
		printf("Se escribio menos (esperada %li, escrito %li)\n",
		(long)lSize/4, frames);

	free(buffer);
	fclose(pFile);
}

void grabar(int volumen, int muestreo, char *archivo)
{
	unsigned int bytes = 123456;
	unsigned int buffer[bytes];

	printf("Grabando %s\n", archivo);
	printf("Volumen: %i\n", volumen);
	printf("Muestreo: %i\n", muestreo);
	err = snd_pcm_open(&handle, device, SND_PCM_STREAM_CAPTURE, 0);
	if (err < 0) {
		printf("Error abriendo el archivo: %s\n", snd_strerror(err));
		exit(EXIT_FAILURE);
	}
	err = snd_pcm_set_params(handle, SND_PCM_FORMAT_S16_LE,
				SND_PCM_ACCESS_RW_INTERLEAVED, 2,
				muestreo, 1, 500000);
	if (err < 0) {
		printf("Error estableciendo los parametros: %s\n",
		snd_strerror(err));
		exit(EXIT_FAILURE);
	}

	printf("Grabando...\n");
	err = snd_pcm_readi(handle, buffer, bytes/4);
	if (err != bytes/4) {
		printf("Fallo la lectura del archivo a grabar\n");
		return;
	}
	printf("Termino de grabar\n");

	pFile = fopen(archivo, "w+");
	if (pFile != NULL)
		fwrite(buffer, sizeof(short), bytes, pFile);
	fclose(pFile);
	printf("Archivo grabado\n");
	snd_pcm_close(handle);
}

int main(int argc, char **argv)
{
	char volumen;
	int muestreo;

	if (argc != 5) {
		printf("Formato incorrecto, utilizar:\n");
		printf("./%s <P | R> <volumen> <frecuencia de muestreo>\
		<archivo de audio>\n", argv[0]);
		return -1;
	}
	volumen = atoi(argv[2]);
	muestreo = atoi(argv[3]);
	if (volumen > 0 && volumen <= 100) {
		SetAlsaMasterVolume(volumen);
	} else {
		printf("Ingresa valor entre 1 y 100 para el volumen\n");
		return -1;
	}
	if (!(muestreo >= 8000 && muestreo <= 48000)) {
		printf("Ingresar valor entre 8000 y 48000 para la\
		frecuencia de muestreo\n");
		return -1;
	}
	if (!strcmp(argv[1], "P")) {
		reproducir(volumen, muestreo, argv[4]);
	} else if (!strcmp(argv[1], "R")) {
		grabar(volumen, muestreo, argv[4]);
	} else {
	    printf("Formato incorrecto, utilizar:\n");
	    printf("./%s <P | R> <volumen> <frecuencia de muestreo>\
		<archivo de audio>\n", argv[0]);
		return -1;
	}

	return 0;
}

