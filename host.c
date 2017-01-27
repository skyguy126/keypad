#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <windows.h>
#include <Winuser.h>
#include "host.h"
#include "secure.h"

static HANDLE hLoopMutex;
static HANDLE hVolumeSema;
static HANDLE hInterruptSema;
static HANDLE hSerial;
static HANDLE hReadThread;
static HANDLE hVolumeThread;
static INPUT keyboardInput;

static volatile int volumeValue;

struct State {
    int key1;
    int key2;
    int key3;
    int key4;
    int var;
};

void initKeyboardInput() {
    keyboardInput.type = INPUT_KEYBOARD;
    keyboardInput.ki.wVk = 0;
    keyboardInput.ki.time = 0;
    keyboardInput.ki.dwExtraInfo = 0;
}

void pressKey(WORD key) {
    keyboardInput.ki.wScan = key;
    keyboardInput.ki.dwFlags = KEYEVENTF_SCANCODE;
    SendInput(1, &keyboardInput, sizeof(INPUT));
}

void releaseKey(WORD key) {
    keyboardInput.ki.wScan = key;
    keyboardInput.ki.dwFlags = KEYEVENTF_KEYUP | KEYEVENTF_SCANCODE;
    SendInput(1, &keyboardInput, sizeof(INPUT));
}

void setVolume(int val) {
    char cmdString[32];
    sprintf(cmdString, "nircmd.exe setsysvolume %d", val);
    system(cmdString);
}

void cancelIoOperation(HANDLE handle) {
    typedef BOOL (*CancelIoEx)(HANDLE hFile, LPOVERLAPPED lpOverlapped);

    CancelIoEx _CancelIoEx;
    HINSTANCE hInstLibrary = LoadLibrary(KERNEL32DLL);

    if (hInstLibrary) {
        _CancelIoEx = (CancelIoEx)GetProcAddress(hInstLibrary, "CancelIoEx");

        if (_CancelIoEx) {
            _CancelIoEx(handle, NULL);
        }

        FreeLibrary(hInstLibrary);
    } else {
        printf("kernel32.dll failed to load\n");
        exit(0);
    }
}

int findPort(char *id) {
    typedef int (*wmicomFunc)(char *);

    wmicomFunc _wmicomFunc;
    HINSTANCE hInstLibrary = LoadLibrary(WMICOM3DLL);

    if (hInstLibrary) {
        _wmicomFunc = (wmicomFunc)GetProcAddress(hInstLibrary, "wmicom");

        // incase there is an error with the function, the library will still
        // be freed and return value will show up as no devices found
        int wmicomFuncOutput = -1;
        if (_wmicomFunc) {
            wmicomFuncOutput = _wmicomFunc(id);
        }

        FreeLibrary(hInstLibrary);
        return wmicomFuncOutput;
    } else {
        printf("wmicom3.dll failed to load\n");
        exit(0);
    }
}

int openSerialPort() {
    char portString[32];
    int devicePort = findPort(DEVICE_ID);

    if (devicePort == -1) {
        return 0;
    }

    DCB dcbSerial = {0};
    COMMTIMEOUTS comTimeouts = {0};

    printf("Opening serial port...   ");
    sprintf(portString, "\\\\.\\COM%d", devicePort);

    // Open serial port
    hSerial = CreateFile(
        portString,                     // COM port number
        GENERIC_READ | GENERIC_WRITE,   // Access mode
        0,                              // Share mode
        NULL,                           // Security
        OPEN_EXISTING,                  // File creation
        FILE_ATTRIBUTE_NORMAL,          // File attributes
        NULL                            // Template
    );

    if (hSerial == INVALID_HANDLE_VALUE) {
        DWORD err, error = GetLastError();
        printf("Error, invalid handle value: %lu\n", error);
        return 0;
    } else {
        printf("OK!\n");
    }

    // Check DCB state
    dcbSerial.DCBlength = sizeof(dcbSerial);
    if (GetCommState(hSerial, &dcbSerial) == 0) {
        printf("Error getting device state\n");
        CloseHandle(hSerial);
        return 0;
    }

    // Set DCB parameters
    dcbSerial.BaudRate = BAUD_RATE;
    dcbSerial.ByteSize = 8;
    dcbSerial.StopBits = ONESTOPBIT;
    dcbSerial.Parity = NOPARITY;
    if(SetCommState(hSerial, &dcbSerial) == 0) {
        printf("Error setting device parameters\n");
        CloseHandle(hSerial);
        return 0;
    }

    // Set timeouts
    comTimeouts.ReadIntervalTimeout = 60000;
    comTimeouts.ReadTotalTimeoutConstant = 0;
    comTimeouts.ReadTotalTimeoutMultiplier = 0;
    comTimeouts.WriteTotalTimeoutConstant = 50;
    comTimeouts.WriteTotalTimeoutMultiplier = 10;
    if(SetCommTimeouts(hSerial, &comTimeouts) == 0) {
        printf("Error setting timeouts\n");
        CloseHandle(hSerial);
        return 0;
    }

    return 1;
}

int closeHandle(HANDLE handle) {
    printf("Closing handle...   ");

    if (CloseHandle(handle) == 0) {
        printf("ERROR!\n");
        return 0;
    } else {
        printf("OK!\n");
    }

    return 1;
}

void clearBuffer(char *buf, int len) {
    memset(buf, 0, len);
}

struct State deserialize(char *buf, int len) {
    struct State state;
    int pass = 0;

    int i = 0;
    char temp[1024];

    for (int x = 0; x < len; x++) {
        if (buf[x] == '$') {
            temp[i+1] = '\0';

            if (pass == 0) {
                state.key1 = atoi(temp);
            } else if (pass == 1) {
                state.key2 = atoi(temp);
            } else if (pass == 2) {
                state.key3 = atoi(temp);
            } else if (pass == 3) {
                state.key4 = atoi(temp);
            } else {
                state.var = atoi(temp);
            }

            i = 0;
            clearBuffer(temp, sizeof(temp));
            pass++;

            continue;
        }

        temp[i] = buf[x];
        i++;
    }

    return state;
}

int readLine(char *buf, int len) {
    char temp[1];

    for (int x = 0; x < len; x++) {
        DWORD bytes_read, total = 0;

        if(!ReadFile(hSerial, temp, 1, &total, NULL)) {
            DWORD err, error = GetLastError();
            printf("Read error: %lu\n", error);
            return 0;
        }

        if (temp[0] != '\n') {
            buf[x] = temp[0];
        } else {
            clearBuffer(temp, sizeof(temp));
            break;
        }

        clearBuffer(temp, sizeof(temp));
    }

    return 1;
}

DWORD WINAPI readSerialLoop(LPVOID lpParam) {
    int lastVolumeValue = 0;
    char buf[READ_BUFFER_SIZE] = {};

    while(1) {

        clearBuffer(buf, sizeof(buf));

        int readSuccess = readLine(buf, sizeof(buf));
        if (!readSuccess) {
            return 0;
        }

        struct State state = deserialize(buf, sizeof(buf));

        printf("Key1: %d Key2: %d Key3: %d Key4: %d Var: %d\n",
        state.key1, state.key2, state.key3, state.key4, state.var);

        if (state.key1) {
            pressKey(0x1E);
        } else {
            releaseKey(0x1E);
        }

        if (state.key2) {
            pressKey(0x30);
        } else {
            releaseKey(0x30);
        }

        if (state.key3) {
            pressKey(0x2E);
        } else {
            releaseKey(0x2E);
        }

        if (state.key4) {
            pressKey(0x20);
        } else {
            releaseKey(0x20);
        }

        WaitForSingleObject(hLoopMutex, INFINITE);
        volumeValue = state.var * 63;
        ReleaseMutex(hLoopMutex);

        if (lastVolumeValue != volumeValue) {
            lastVolumeValue = volumeValue;
            ReleaseSemaphore(hVolumeSema, 1, NULL);
        }

        clearBuffer(buf, sizeof(buf));
    }

    return 1;
}

DWORD WINAPI updateVolumeLoop(LPVOID lpParam) {
    int curVolVal = 0;

    while(1) {

        WaitForSingleObject(hVolumeSema, INFINITE);

        WaitForSingleObject(hLoopMutex, INFINITE);
        curVolVal = volumeValue;
        ReleaseMutex(hLoopMutex);

        setVolume(curVolVal);
    }

    return 1;
}

void interruptHandler(int state) {
    printf("\nExiting...\n");

    // prevent main loop from restarting
    WaitForSingleObject(hInterruptSema, INTERRUPT_DELAY);

    cancelIoOperation(hSerial);
    closeHandle(hSerial);
    closeHandle(hReadThread);
    closeHandle(hVolumeThread);
    closeHandle(hLoopMutex);
    closeHandle(hVolumeSema);

    exit(0);
}

int main(int argc, char **argv) {

    system("cls");
    signal(SIGINT, interruptHandler);
    initKeyboardInput();

    hLoopMutex = CreateMutex(
        NULL,
        FALSE,
        NULL
    );

    hVolumeSema = CreateSemaphore(
        NULL,
        0,
        1,
        NULL
    );

    hInterruptSema = CreateSemaphore(
        NULL,
        1,
        1,
        NULL
    );

    DWORD volumeThreadId, volumeThreadParam = 1;
    hVolumeThread = CreateThread(
        NULL,
        0,
        updateVolumeLoop,
        &volumeThreadParam,
        0,
        &volumeThreadId
    );

    while(1) {
        Sleep(RELOAD_DELAY);
        
        int portOpenSuccess = openSerialPort();
        if (!portOpenSuccess) {
            continue;
        }

        system("start toast.exe");

        DWORD readThreadId, readThreadParam = 1;
        hReadThread = CreateThread(
            NULL,
            0,
            readSerialLoop,
            &readThreadParam,
            0,
            &readThreadId
        );

        WaitForSingleObject(hReadThread, INFINITE);

        // when an interrupt event happens do not proceed with this loop
        // wait till program exits
        WaitForSingleObject(hInterruptSema, INFINITE);
        ReleaseSemaphore(hInterruptSema, 1, NULL);

        cancelIoOperation(hSerial);
        closeHandle(hSerial);
        closeHandle(hReadThread);
    }

    return 0;
}
