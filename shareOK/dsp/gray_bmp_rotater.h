#ifndef _GRAY_BMP_ROTATER_H_
#define _GRAY_BMP_ROTATER_H_

#include <stdint.h>
#include <stdbool.h>

#define PI 3.1415927

typedef struct _GrayBmpRotater GrayBmpRotater;

GrayBmpRotater *gray_bmp_rotater_new();
void            gray_bmp_rotater_destroy(GrayBmpRotater *rotater);
bool            gray_bmp_rotater_process(GrayBmpRotater *rotater, uint8_t angle, uint8_t *input, uint8_t *output);

#endif
