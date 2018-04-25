#define CODE_IN_IWRAM __attribute__ ((section (".iwram"), long_call))
#define VAR_IN_IWRAM __attribute__ ((section (".iwram"))) = {0}
