#ifndef APP_H
#define APP_H

#define DEVICE "/dev/PARALLEL_DEV0"

void parallel_config_output();
void parallel_config_input();
void parallel_write(unsigned char value);
char parallel_read();
void blink(void);

#endif /* APP_H */
