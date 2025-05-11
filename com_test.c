#include <windows.h>
#include <setupapi.h>
#include <initguid.h>
#include <devguid.h>
#include <regstr.h>
#include <stdio.h>

#pragma comment(lib, "setupapi.lib")

void list_com_ports() {
    char port_name[10];
    for (int i = 1; i <= 20; ++i) {
        sprintf(port_name, "\\\\.\\COM%d", i);  // Use special syntax for COM>9
        HANDLE h = CreateFileA(port_name, GENERIC_READ | GENERIC_WRITE,
                               0, NULL, OPEN_EXISTING, 0, NULL);
        if (h != INVALID_HANDLE_VALUE) {
            printf("Found: %s\n", port_name);
            CloseHandle(h);
        }
    }
}

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
            printf("%s\n", friendlyName);
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

#include <conio.h>  // _kbhit, _getch

#define ACK 0x06
#define SHUTDOWN 0xFF

int consume_all_chars(HANDLE hSerial) {
    DWORD bytesRead;
    BYTE b;
    
    // Keep reading until there's no more data in the buffer
    while (1) {
        if (ReadFile(hSerial, &b, 1, &bytesRead, NULL) && bytesRead == 1) {
            // You can optionally log the discarded characters if needed
            // printf("Discarded byte: 0x%02X\n", b); // for debugging
        } else {
            break; // No more data to read
        }
    }

    return 0; // All characters consumed
}
int wait_for_ack(HANDLE hSerial, DWORD timeout_ms) {
    DWORD start = GetTickCount();
    DWORD bytesRead;
    BYTE b;

    while ((GetTickCount() - start) < timeout_ms) {
        if (ReadFile(hSerial, &b, 1, &bytesRead, NULL) && bytesRead == 1) {
            if (b == ACK){
                printf("acknowledged\n");
                if (ReadFile(hSerial, &b, 1, &bytesRead, NULL) && bytesRead == 1) {
                    printf("char returned: %c, %d\n", b, b);

                    if(b == VK_ESCAPE) {
                        if (ReadFile(hSerial, &b, 1, &bytesRead, NULL) && bytesRead == 1) {
                            if(b == SHUTDOWN) {
                                printf("Shutdown signal received");
                                exit(0);
                            }
                        }                        
                    }
                }
                return 1; // ACK received
            }
        }
        Sleep(1); // avoid spinning too hard
    }

    return 0; // timeout
}

void send_keys_over_uart(HANDLE hSerial) {
    printf("send anything\n");
    while (1) {
        if (_kbhit()) {
            char c = _getch();
            DWORD written;
            WriteFile(hSerial, &c, 1, &written, NULL);
            if(!wait_for_ack(hSerial, 500)) {
                printf("connection timed out\n");
                exit(1);
            }
        }
        Sleep(10);  // Avoid 100% CPU
    }
}


int main() {
    list_serial_ports_with_names();
    const char *port = "\\\\.\\COM5";  // Replace with actual detected port
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
        
    while(1) {
        printf("loop\n");
        send_keys_over_uart(hSerial);
    }
    return 0;
}