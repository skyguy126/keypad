#pragma once

#define DEBUG_MODE

#define BAUD_RATE CBR_19200
#define VOLUME_MULTIPLIER 63
#define READ_BUFFER_SIZE 1024
#define RELOAD_DELAY 5000
#define INTERRUPT_DELAY 1500
#define KERNEL32DLL "Kernel32.dll"
#define WMICOM3DLL "wmicom3.dll"

#ifndef DEBUG_MODE
    #define printf(fmt, ...) (0)
#endif
