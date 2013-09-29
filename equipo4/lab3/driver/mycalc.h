#ifndef MYCALC_H
#define MYCALC_H

#define MYCALC_IOC_MAGIC	        'x'
#define	MYCALC_IOC_SET_NUM1	        _IOW(MYCALC_IOC_MAGIC, 0, int)
#define	MYCALC_IOC_SET_NUM2		    _IOW(MYCALC_IOC_MAGIC, 1, int)
#define	MYCALC_IOC_SET_OPERATION	_IOW(MYCALC_IOC_MAGIC, 2, int)
#define	MYCALC_IOC_GET_RESULT		_IOR(MYCALC_IOC_MAGIC, 3, int)
#define	MYCALC_IOC_DO_OPERATION		_IOWR(MYCALC_IOC_MAGIC, 4, int)

enum operator_type_t{
	MYCALC_ADD = 0,
	MYCALC_STR = 1,
	MYCALC_DIV = 2,
	MYCALC_MUL = 3
};

struct operation_t{
	int num1;
	int num2;
    enum operator_type_t operator_type;
	int result;
};

int get_definition_operator(char optor);
int get_lenght_number(int number);
static int validate_input(char *input, int lenght);
int validate_number(int value, int min, int max);
int compute_operation(struct operation_t *operation);

#endif /* MYCALC_H */
