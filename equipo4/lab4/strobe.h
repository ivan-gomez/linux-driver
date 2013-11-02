#ifndef STROBE_H
#define STROBE_H

#define DEVICE "/dev/PARALLEL_DEV0"

void parallel_config_output();
void parallel_config_input();
void parallel_write(char value);
char parallel_read();

#endif /* STROBE_H */
