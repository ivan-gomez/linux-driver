#ifndef ALSAPLAYER_H
#define ALSAPLAYER_H

int get_fd_file(char action, char *file);
int get_errors_params_validation(char action, int volume, int channels,
	int rate, int seconds, int fd_file);
void print_summary_params(char action, int volume, int channels,
	int rate, int seconds);
void set_master_volume(long volume);
snd_pcm_stream_t get_pcm_stream(char action);
void configure_pcm(snd_pcm_t **pcm_handle, snd_pcm_hw_params_t *params,
	char action, int channels, int rate, int volume);

#endif /* ALSAPLAYER_H */
