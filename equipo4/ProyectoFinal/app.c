#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "driver/gaming.h"
#include "app.h"

int main()
{
	int rc, fd, rd;
	unsigned char data[GAMING_DRIVER_PCK_SIZE]; 

	system("clear");
	fd = open(DEVICE, O_RDONLY);
	if (fd < 0) {
		printf("Error opening device\n");
		return fd;
	}
	printf("-----Gaming driver-----\n");
	printf("Connection open\n");
	printf(" 1) Ensure analog mode is ON\n");
	printf(" 2) Press button 10 to exit\n\n");
	printf("--Events--\n");

	do {
		rd = read(fd, &data, GAMING_USB_PCK_SIZE);
		if (IS_ANALOG(data))
			performActions(data);
	} while (!IS_BUTTON_10(data));

	rc = close(fd);
	if (rc < 0) {
		printf("Error closing device\n");
		return rc;
	}
	printf("Connection was closed\n");

	return 0;
}

/*
 * performActions()
 *  Evaluate all keyboard and mouse events
 * Parameters:
 *  char* - data from device
 * Returns:
 *  -none-
 */
void performActions(char *data)
{

	/* Mouse joystick */
	mouse_move(IS_JOYSTICK_UP(data), IS_JOYSTICK_DOWN(data),
	IS_JOYSTICK_LEFT(data), IS_JOYSTICK_RIGHT(data));

	/* Mouse clicks */
	mouse_click(IS_BUTTON_3(data), &click_mode_left, 1);
	mouse_click(IS_BUTTON_4(data), &click_mode_right, 3);

	/* Keys events */
	keyboard_press(IS_CURSOR_UP(data), &key_up, "Up");
	keyboard_press(IS_CURSOR_DOWN(data), &key_down, "Down");
	keyboard_press(IS_CURSOR_LEFT(data), &key_left, "Left");
	keyboard_press(IS_CURSOR_RIGHT(data), &key_right, "Right");

	keyboard_press(IS_BUTTON_6(data), &key_pprior, "Prior");
	keyboard_press(IS_BUTTON_8(data), &key_pnext, "Next");

	keyboard_press(IS_BUTTON_1(data), &key_return, "Return");
	keyboard_press(IS_BUTTON_2(data), &key_escape, "Escape");

	/* Keys speed config */
	mouse_speed(IS_BUTTON_5(data), &speed_decrement, 1, "Increment");
	mouse_speed(IS_BUTTON_7(data), &speed_increment, -1, "Decrement");
}

/*
 * mouse_move()
 *  Function to make a call for moving the mouse with joystick
 * Parameters:
 *  int up - 1 or 0 if up joystick was moved
 *  int down - 1 or 0 if down joystick was moved
 *  int left - 1 or 0 if left joystick was moved
 *  int right - 1 or 0 if right joystick was moved
 * Returns:
 *  -none-
 */
void mouse_move(int up, int down, int left, int right)
{
	int i;
	if ((up + down + left + right) > 0) {
		sprintf(cmd, "xdotool mousemove_relative -- %d %d",
			(right + (left*-1)) * (step_size),
			(down + (up*-1)) * (step_size));
		for (i = 0; i < STEP_N; i++)
			system(cmd);
	}
}

/*
 * mouse_click()
 *  Perform mouse click events
 * Parameters:
 *  int isPressed - 1 or 0 if left/right click is pressed
 *  int* click_mode - Pointer to satus mode variable left/right
 *  int button - Select button id to move 1-left/3-right
 * Returns:
 *  -none-
 */
void mouse_click(int isPressed, int *click_mode, int button)
{
	if (isPressed && *click_mode == UNPRESSED) {
		*click_mode = PRESSED;
		sprintf(cmd, "xdotool mousedown %d", button);
		printf("button_%d_mousedown()\n", button);
		system(cmd);
	} else if (!isPressed && *click_mode == PRESSED) {
		*click_mode = UNPRESSED;
		sprintf(cmd, "xdotool mouseup %d", button);
		printf("button_%d_mouseup()\n", button);
		system(cmd);
	}
}

/*
 * keyboard_press()
 *  Perform keys events
 * Parameters:
 *  int isPressed - 1 or 0 if the key is pressed
 *  int* press_mode - Pointer to the key satus mode variable
 *  const char *keystroke - String of the pressed key according to xdotool library
 * Returns:
 *  -none-
 */
void keyboard_press(int isPressed, int *press_mode, const char *keystroke)
{
	if (isPressed && *press_mode == UNPRESSED) {
		*press_mode = PRESSED;
		sprintf(cmd, "xdotool keydown %s", keystroke);
		printf("key_%s_keydown()\n", keystroke);
		system(cmd);
	} else if (!isPressed && *press_mode == PRESSED) {
		*press_mode = UNPRESSED;
		sprintf(cmd, "xdotool keyup %s", keystroke);
		printf("key_%s_keyup()\n", keystroke);
		system(cmd);
	}
}

/*
 * mouse_speed()
 *  Increase or decrease mouse speed navigation
 * Parameters:
 *  int isPressed - 1 or 0 if the key is pressed
 *  int* press_mode - Pointer to the speed key satus mode variable
 *  const char *msg - String indicating which event was raised (increment/decrement)
 * Returns:
 *  -none-
 */
void mouse_speed(int isPressed, int *press_mode, int step, const char *msg)
{
	if (isPressed && *press_mode == UNPRESSED) {
		*press_mode = PRESSED;
		if (step_size + step <= 10 && step_size + step > 0)
			step_size += step;
		printf("speed_%s() = %d\n", msg, step_size);
	} else if (!isPressed && *press_mode == PRESSED)
		*press_mode = UNPRESSED;
}

