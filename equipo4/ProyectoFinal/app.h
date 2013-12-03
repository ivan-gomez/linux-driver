#ifndef APP_H
#define APP_H

#define STEP_N		5
#define DEVICE		"/dev/gaming0"

#define PRESSED		1
#define UNPRESSED	0

char cmd[100];
int step_size = 3; /* Mouse speed from 1 to 9 */

/* Button click modes */ 
int click_mode_left = UNPRESSED;
int click_mode_right = UNPRESSED;

/* Keyboard modes keys */
int key_up = UNPRESSED;
int key_down = UNPRESSED;
int key_left = UNPRESSED;
int key_right = UNPRESSED;
int key_pprior = UNPRESSED;
int key_pnext = UNPRESSED;
int key_return = UNPRESSED;
int key_escape = UNPRESSED;

/* Speed modes keys */
int speed_decrement = UNPRESSED;
int speed_increment = UNPRESSED;

/* Prototypes */
void performActions(char *data);
void mouse_move(int up, int down, int left, int right);
void mouse_click(int isPressed, int *click_mode, int button);
void mouse_speed(int isPressed, int *press_mode, int step, const char *msg);
void keyboard_press(int isPressed, int *press_mode, const char *keystroke);

#endif /* APP_H */
