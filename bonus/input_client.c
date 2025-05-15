#include <windows.h>
#include <setupapi.h>
#include <initguid.h>
#include <devguid.h>
#include <regstr.h>
#include <stdio.h>
#include <stdbool.h>
#pragma comment(lib, "setupapi.lib")

void list_serial_ports_with_names() {
    HDEVINFO hDevInfo = SetupDiGetClassDevs(
        &GUID_DEVCLASS_PORTS, 0, 0, DIGCF_PRESENT);

    if (hDevInfo == INVALID_HANDLE_VALUE) return;

    SP_DEVINFO_DATA devInfoData;
    devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

    for (DWORD i = 0; SetupDiEnumDeviceInfo(hDevInfo, i, &devInfoData); ++i) {
        char friendlyName[256];
        if (SetupDiGetDeviceRegistryPropertyA(hDevInfo, &devInfoData,
                SPDRP_FRIENDLYNAME, NULL,
                (PBYTE)friendlyName, sizeof(friendlyName), NULL)) {
            printf("\t%s\n", friendlyName);
        }
    }

    SetupDiDestroyDeviceInfoList(hDevInfo);
}

HANDLE open_serial_port(const char *port_name) {
    HANDLE hSerial = CreateFileA(port_name, GENERIC_READ | GENERIC_WRITE,
                                 0, 0, OPEN_EXISTING, 0, 0);

    if (hSerial == INVALID_HANDLE_VALUE) {
        printf("Error opening port %s\n", port_name);
        return INVALID_HANDLE_VALUE;
    }

    DCB dcbSerialParams = {0};
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
    GetCommState(hSerial, &dcbSerialParams);

    dcbSerialParams.BaudRate = CBR_115200;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity   = NOPARITY;
    SetCommState(hSerial, &dcbSerialParams);

    COMMTIMEOUTS timeouts = {0};
    timeouts.ReadIntervalTimeout = 50;
    timeouts.ReadTotalTimeoutConstant = 50;
    timeouts.ReadTotalTimeoutMultiplier = 10;
    timeouts.WriteTotalTimeoutConstant = 50;
    timeouts.WriteTotalTimeoutMultiplier = 10;
    SetCommTimeouts(hSerial, &timeouts);

    return hSerial;
}


bool do_print;

#define ACK 0x06
#define SHUTDOWN 0xFF
#define SEND_INPUTSTATE 0xFE

void consume_chars(HANDLE hSerial) {
    printf("begin reading\n");
    DWORD bytesRead;
    BYTE b;
    
    //consume a maximum of 1000 characters to avoid permanently stalling if the rpi is spamming the port;
    int i = 0;
    while (i++ < 1000) {
        if (ReadFile(hSerial, &b, 1, &bytesRead, NULL) && bytesRead == 1) {
            // You can optionally log the discarded characters if needed
            if(do_print)
                printf("%c", b); // for debugging
        } else {
            break; // No more data to read
        }
    }
    printf("end reading\n");
}

void consume_all_chars(HANDLE hSerial) {
    DWORD bytesRead;
    BYTE b;
    
    while (1) {
        if (ReadFile(hSerial, &b, 1, &bytesRead, NULL) && bytesRead == 1) {
                printf("%c", b);
        } else {
            break; // No more data to read
        }
    }
}

int wait_for_ack(HANDLE hSerial, DWORD timeout_ms) {
    DWORD start = GetTickCount();
    DWORD bytesRead;
    BYTE b;

    while ((GetTickCount() - start) < timeout_ms) {
        if (ReadFile(hSerial, &b, 1, &bytesRead, NULL) && bytesRead == 1) {
            if (b == ACK){
                printf("acknowledged\n");
                return 1; // ACK received
            }
        }
        Sleep(5); // avoid spinning too hard
    }

    printf("ack not received\n");
    return 0; // timeout
}

typedef struct {
    bool up, down, left, right;
    bool fire;
    bool start;
} Input;

Input old_input;
Input new_input;

void print_input(Input input) {
    printf("up: %d, down: %d, left: %d, right: %d, fire: %d, start: %d\n",
        input.up,
        input.down,
        input.left,
        input.right,
        input.fire,
        input.start
    );
}

void communicate(HANDLE hSerial) {
    unsigned char c;
    DWORD written;
    DWORD bytesRead;
    BYTE b;

    //consume_chars(hSerial);

    if(GetAsyncKeyState(VK_ESCAPE)) {
        printf("sending shutdown signal\n");
        c = VK_ESCAPE;
        WriteFile(hSerial, &c, 1, &written, NULL);
        if(!wait_for_ack(hSerial, 2500)) {
            printf("connection timed out\n");
            exit(1);
        }

        if (ReadFile(hSerial, &b, 1, &bytesRead, NULL) && bytesRead == 1) {
            if(b == SHUTDOWN) {
                printf("shutdown signal received by pi\n");
                exit(0);
            }
        }        
    }

    //handle game input
    {
        //get input state
        {
            new_input.up = GetAsyncKeyState(VK_UP);
            new_input.down = GetAsyncKeyState(VK_DOWN);
            new_input.left = GetAsyncKeyState(VK_LEFT);
            new_input.right = GetAsyncKeyState(VK_RIGHT);
            new_input.fire = GetAsyncKeyState('F');
            new_input.start = GetAsyncKeyState(VK_SPACE);
        }

        //check dirty
        {
            bool dirty =
            (new_input.up    != old_input.up)    ||
            (new_input.down  != old_input.down)  ||
            (new_input.left  != old_input.left)  ||
            (new_input.right != old_input.right) ||
            (new_input.fire  != old_input.fire)  ||
            (new_input.start != old_input.start);

            if(dirty) {
                printf("fresh input!\n");
                printf("old input state:\n");
                print_input(old_input);
                printf("new input state:\n");
                print_input(new_input);
                unsigned char request[7];
                FlushFileBuffers(hSerial);
                request[0] = SEND_INPUTSTATE;
                request[1] = new_input.up;
                request[2] = new_input.down;
                request[3] = new_input.left;
                request[4] = new_input.right;
                request[5] = new_input.fire;
                request[6] = new_input.start;


                WriteFile(hSerial, &request[0], 1, &written, NULL);
                if (ReadFile(hSerial, &b, 1, &bytesRead, NULL) && bytesRead == 1) {
                    if (b != ACK){
                        printf("problem\n");
                    }
                }
                WriteFile(hSerial, &request[1], 1, &written, NULL);
                if (ReadFile(hSerial, &b, 1, &bytesRead, NULL) && bytesRead == 1) {
                    if (b != ACK){
                        printf("problem\n");
                    }
                }
                WriteFile(hSerial, &request[2], 1, &written, NULL);
                if (ReadFile(hSerial, &b, 1, &bytesRead, NULL) && bytesRead == 1) {
                    if (b != ACK){
                        printf("problem\n");
                    }
                }
                WriteFile(hSerial, &request[3], 1, &written, NULL);
                if (ReadFile(hSerial, &b, 1, &bytesRead, NULL) && bytesRead == 1) {
                    if (b != ACK){
                        printf("problem\n");
                    }
                }
                WriteFile(hSerial, &request[4], 1, &written, NULL);
                if (ReadFile(hSerial, &b, 1, &bytesRead, NULL) && bytesRead == 1) {
                    if (b != ACK){
                        printf("problem\n");
                    }
                }
                WriteFile(hSerial, &request[5], 1, &written, NULL);
                if (ReadFile(hSerial, &b, 1, &bytesRead, NULL) && bytesRead == 1) {
                    if (b != ACK){
                        printf("problem\n");
                    }
                }
                WriteFile(hSerial, &request[6], 1, &written, NULL);
                if (ReadFile(hSerial, &b, 1, &bytesRead, NULL) && bytesRead == 1) {
                    if (b != ACK){
                        printf("problem\n");
                    }
                }

            }
        }

        old_input = new_input;
    }

    Sleep(5);  // avoid spinning too hard.
}


int main(int argc, char **argv) {
    for (int i = 0; i < argc; ++i)
    {
        if(strcmp(argv[i], "--print") == 0) {
            printf("printing incoming data from pi enabled\n");
            do_print = true; //not very useful as printing in windows terminal is far too slow
        }
    }

    printf("Available ports: \n");
    list_serial_ports_with_names();

    printf("select your com port:\n");
    unsigned char c;
    scanf_s("%c", &c);
    char port[11];
    sprintf(port, "\\\\.\\COM%c", c);
    
    HANDLE hSerial = open_serial_port(port);
    if (hSerial == INVALID_HANDLE_VALUE) { 
        printf("No connection");
        return 1;
    }


    COMMTIMEOUTS timeouts;

    // Set the timeouts for the serial port
    if (!SetCommTimeouts(hSerial, &timeouts)) {
        printf("Failed to set timeouts.\n");
        CloseHandle(hSerial);
        return 1;
    }
    
    printf("successfully connected to COM%c\n", c);

    Sleep(150);
    //get input state
    {
        new_input.up = GetAsyncKeyState(VK_UP);
        new_input.down = GetAsyncKeyState(VK_DOWN);
        new_input.left = GetAsyncKeyState(VK_LEFT);
        new_input.right = GetAsyncKeyState(VK_RIGHT);
        new_input.fire = GetAsyncKeyState('F');
        new_input.start = GetAsyncKeyState(VK_SPACE);
    }
    Sleep(150);
    //get input state
    {
        new_input.up = GetAsyncKeyState(VK_UP);
        new_input.down = GetAsyncKeyState(VK_DOWN);
        new_input.left = GetAsyncKeyState(VK_LEFT);
        new_input.right = GetAsyncKeyState(VK_RIGHT);
        new_input.fire = GetAsyncKeyState('F');
        new_input.start = GetAsyncKeyState(VK_SPACE);
    }

    while(1) {
        communicate(hSerial);
    }
    return 0;
}