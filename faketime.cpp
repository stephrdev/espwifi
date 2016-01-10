#include "faketime.h"


struct timeval tp_boot;


uint16_t time_ms(uint8_t init) {
	struct timeval tp;
	if (init) {
		gettimeofday(&tp_boot, 0);
	}

	gettimeofday(&tp, 0);

	return (
		((tp.tv_sec * 1000) + (tp.tv_usec / 1000))
		- ((tp_boot.tv_sec * 1000) + (tp_boot.tv_usec / 1000))
	);
}

uint16_t time_ms() {
	return time_ms(0);
}
