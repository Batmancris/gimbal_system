#ifndef COORDINATE_H
#define COORDINATE_H

#include "main.h"

typedef struct
{
    volatile uint8_t data_ready;
    volatile uint16_t object_x;
    volatile uint16_t object_y;
} TargetPosition;

extern TargetPosition target_position;

const TargetPosition *get_target_position(void);
uint8_t fetch_target_position(uint16_t *x, uint16_t *y);
void set_target_position(uint16_t x, uint16_t y);
void set_target_data_ready(uint8_t ready);

#endif
