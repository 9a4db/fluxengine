#ifndef GLOBALS_H
#define GLOBALS_H

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "protocol.h"

extern void error(const char* message, ...);

extern void open_serial_port(const char* port);
extern void read_frame(frame_t* frame);

#endif
