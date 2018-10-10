#ifndef GLOBALS_H
#define GLOBALS_H

#define _BSD_SOURCE

#if defined _WIN32 || defined __CYGWIN__
    #define WINDOWS
#endif

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <endian.h>

#include "protocol.h"

struct encoding_buffer
{
    int length_us;
    uint8_t* bitmap;
};

extern struct encoding_buffer* create_encoding_buffer(int length_us);
extern void free_encoding_buffer(struct encoding_buffer* buffer);

extern void encoding_buffer_pulse(struct encoding_buffer* buffer, int timestamp_us);
extern struct fluxmap* encoding_buffer_encode(const struct encoding_buffer* buffer);

struct fluxmap
{
    int length_ticks;
    int length_us;
    int bytes;
    int buffersize;
    uint8_t* intervals;
};

extern struct fluxmap* create_fluxmap(void);
extern void free_fluxmap(struct fluxmap* fluxmap);
extern void fluxmap_clear(struct fluxmap* fluxmap);
extern void fluxmap_append_intervals(struct fluxmap* fluxmap, const uint8_t* intervals, int count);

extern void error(const char* message, ...);
extern double gettime(void);
extern int countargs(char* const* argv);

extern void usb_init(void);
extern void usb_cmd_send(void* ptr, int len);
extern void usb_cmd_recv(void* ptr, int len);

extern int usb_get_version(void);
extern void usb_seek(int track);
extern int usb_measure_speed(void);
extern void usb_bulk_test(void);
extern struct fluxmap* usb_read(int side);
extern int usb_write(int side, struct fluxmap* fluxmap);

extern void cmd_rpm(char* const* argv);
extern void cmd_usbbench(char* const* argv);
extern void cmd_read(char* const* argv);
extern void cmd_write(char* const* argv);
extern void cmd_decode(char* const* argv);
extern void cmd_testpattern(char* const* argv);
extern void cmd_fluxdump(char* const* argv);

#endif
