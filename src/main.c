#include <stdio.h>
#include <string.h>
#include <windows.h>
#include "capture.h"

int main(int argc, char *argv[]) {

    // Tells Windows to send us the real pixel dimensions instead of returning scaled-down values.
    SetProcessDPIAware();

    // Checks if the user provided an argument
    if (argc < 2) {
        printf("Usage: rdp.exe --host | --client\n");
        return 1;
    }

    // Checks if the argument is --host
    if (strcmp(argv[1], "--host") == 0) {
        printf("[HOST] Starting...\n");
        
        // Loop that continuously captures screenshots
        while (1) {
            capture_screenshot("screenshot.bmp");
            Sleep(20); 
        }

    // Checks if the argument is --client
    } else if (strcmp(argv[1], "--client") == 0) {
        printf("[CLIENT] Starting...\n");
        printf("(client not implemented yet)\n");
    // Checks if the argument is neither --host nor --client
    } else {
        printf("Unknown mode: %s\n", argv[1]);
        return 1;
    }

    return 0;
}