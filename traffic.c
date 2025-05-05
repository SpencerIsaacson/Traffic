#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#include "traffic.h"
#include "tests.h"

char *light_state_name_lookup[LightState_count] = {
	[LightState_RED]    =    "Red",
	[LightState_YELLOW] = "Yellow",
	[LightState_GREEN]  =  "Green",
};

char *light_state_ansi_lookup[LightState_count] = {
	[LightState_RED]    = "\033[0;41mR\033[0m",
	[LightState_YELLOW] = "\033[0;43mY\033[0m",
	[LightState_GREEN]  = "\033[0;42mG\033[0m",
};


/* clears the terminal and resets the cursor position to 0, 0 */
void clear_console() {
	printf("\033[2J\033[H");
}

void set_cursor_position(unsigned char x, unsigned char y) {
	printf("\033[%d;%dH", y, x);
}


void enable_ansi_codes() {
#ifdef _WIN32
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif	
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);
#endif
}

#define FANCY_PRINT 1

/*for the pun of it*/
void printersection(Intersection *intersection) {
	set_cursor_position(0, 0);
	printf("Traffic State:\n");
	set_cursor_position(7, 2);
	printf("%s",light_state_ansi_lookup[intersection->lights[Direction_NorthSouth]]);
	set_cursor_position(9, 2);
	printf("%s",light_state_ansi_lookup[intersection->lights[Direction_NorthSouth]]);
	set_cursor_position(4, 8);
	printf("%s",light_state_ansi_lookup[intersection->lights[Direction_NorthSouth]]);
	set_cursor_position(6, 8);
	printf("%s",light_state_ansi_lookup[intersection->lights[Direction_NorthSouth]]);

	set_cursor_position(1, 3);
	printf("%s",light_state_ansi_lookup[intersection->lights[Direction_EastWest]]);
	set_cursor_position(1, 4);
	printf("%s",light_state_ansi_lookup[intersection->lights[Direction_EastWest]]);
	set_cursor_position(12, 5);
	printf("%s",light_state_ansi_lookup[intersection->lights[Direction_EastWest]]);
	set_cursor_position(12, 6);
	printf("%s",light_state_ansi_lookup[intersection->lights[Direction_EastWest]]);
	printf("\n");
	fflush(stdout);
}

int main(int argc, char **argv) {
	enable_ansi_codes();
	clear_console();
	if(argc > 1 && strcmp(argv[1], "--test") == 0) {
		run_test_suite();
		return (tests_run - tests_passed);
	}
	else {
		while(1) {
			static clock_t last_print = 0;
			if((clock()-last_print) > (CLOCKS_PER_SEC/10)){
#if FANCY_PRINT
				printersection(&intersection);
#else
				printf("north-south: %s, east-west: %s\n", 
					light_state_name_lookup[intersection.lights[Direction_NorthSouth]],
					light_state_name_lookup[intersection.lights[Direction_EastWest]]);
#endif
				last_print = clock();
			}

			traffic_strategy(&intersection);
			assert(at_least_one_axis_red);

#if _WIN32
			if(GetAsyncKeyState(VK_ESCAPE)) {
				clear_console();
				return 0;
			}
#endif			
		}
	}

	return 0;
}