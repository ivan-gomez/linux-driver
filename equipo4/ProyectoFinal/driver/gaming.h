#ifndef GAMING_H
#define GAMING_H


/* Buttons macros */
#define IS_BUTTON_1(byte)	((*(byte+1) & 0x01) > 0)
#define IS_BUTTON_2(byte)	((*(byte+1) & 0x02) > 0)
#define IS_BUTTON_3(byte)	((*(byte+1) & 0x04) > 0)
#define IS_BUTTON_4(byte)	((*(byte+1) & 0x08) > 0)

#define IS_BUTTON_5(byte)	((*(byte+1) & 0x10) > 0)
#define IS_BUTTON_6(byte)	((*(byte+1) & 0x20) > 0)
#define IS_BUTTON_7(byte)	((*(byte+1) & 0x40) > 0)
#define IS_BUTTON_8(byte)	((*(byte+1) & 0x80) > 0)

#define IS_ANALOG(byte)		((*(byte+2) & 0x01) > 0)
#define IS_BUTTON_9(byte)	((*(byte+2) & 0x02) > 0)
#define IS_BUTTON_10(byte)	((*(byte+2) & 0x04) > 0)

/* Cursor macros */
#define IS_CURSOR_UP(byte)	    ((*byte & 0x10) > 0)
#define IS_CURSOR_DOWN(byte)	((*byte & 0x20) > 0)
#define IS_CURSOR_LEFT(byte)	((*byte & 0x40) > 0)
#define IS_CURSOR_RIGHT(byte)	((*byte & 0x80) > 0)

#define IS_JOYSTICK_UP(byte)	((*byte & 0x01) > 0)
#define IS_JOYSTICK_DOWN(byte)	((*byte & 0x02) > 0)
#define IS_JOYSTICK_LEFT(byte)	((*byte & 0x04) > 0)
#define IS_JOYSTICK_RIGHT(byte)	((*byte & 0x08) > 0)

#define GAMING_USB_PCK_SIZE	8
#define GAMING_DRIVER_PCK_SIZE	3

#endif /* GAMING_H */
