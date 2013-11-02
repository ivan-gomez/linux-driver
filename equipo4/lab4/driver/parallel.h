#ifndef PARALLEL_H
#define PARALLEL_H

#define PARALLEL_IOC_MAGIC		'x'
#define	PARALLEL_SET_OUTPUT		_IO(PARALLEL_IOC_MAGIC, 0)
#define	PARALLEL_SET_INPUT		_IO(PARALLEL_IOC_MAGIC, 1)
#define	PARALLEL_STROBE_ON		_IO(PARALLEL_IOC_MAGIC, 2)
#define	PARALLEL_STROBE_OFF		_IO(PARALLEL_IOC_MAGIC, 3)

#define READ_MODE		0
#define WRITE_MODE		1

#define OFF			0
#define ON			1

#define CTRL_DIR_IN		(1 << 5)
#define CTRL_IRQ_ON		(1 << 4)
#define CTRL_STROBE_OFF		(1 << 0)

void config_parallel_input(void);
void config_parallel_output(void);
void simulate_parallel_irq_strb_on(void);
void simulate_parallel_irq_strb_off(void);
void resetInterruptIRQ(void);

#endif /* PARALLEL_H */
