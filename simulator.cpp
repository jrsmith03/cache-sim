#define KHz(num) ((num)*1000UL)
#define MHz(num) (KHz(num)*1000UL)
#define GHz(num) (MHz(num)*1000UL)

#define L1_CYCLE_DELAY 1
#define L2_CYCLE_DELAY 10
#define DRAM_CYCLE_DELAY 100

#define CLOCK_SPEED GHz(2)