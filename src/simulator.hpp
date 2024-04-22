#define KHz(num) ((num)*1000UL)
#define MHz(num) (KHz(num)*1000UL)
#define GHz(num) (MHz(num)*1000UL)

#define KiB(num) ((num)*1024UL)
#define MiB(num) (KiB(num)*1024UL)
#define GiB(num) (MiB(num)*1024UL)

using Freq = u64;
const Freq CLOCK_SPEED = GHz(2);
const Time CYCLE_TIME = s(1) / CLOCK_SPEED;
