#include "traffic.h"

int tests_run = 0;
int tests_passed = 0;

/* wrapped in do while so macro expansion compiles correctly wherever it's put */
#define RUN_TEST(test) do { \
	tests_run++; \
	if(!test()) \
		printf("\033[31mFAILURE\033[0m %s, %d: %s\n", __FILE__, __LINE__, #test); \
	else { \
		printf("\033[32mPASS\033[0m: %s\n", #test); \
		tests_passed++;\
	} \
	\
} while(0)

bool all_lights_red() {
	for (int i = 0; i < Direction_count; ++i) {
		if(intersection.lights[i] != LightState_RED)
			return false;
	}

	return true;	
}

bool all_lights_set_to_stop_on_startup() {
	return inited == false && all_lights_red();
}

bool initialize_resets_to_stop() {
	intersection.lights[Direction_NorthSouth] = LightState_GREEN;
	intersection.lights[Direction_EastWest]   = LightState_GREEN;
	init_intersection(&intersection);
	return all_lights_red();
}

bool north_south_is_yellow_after_its_green() {
	while(intersection.lights[Direction_NorthSouth] != LightState_GREEN)
		next_state(&intersection);
	next_state(&intersection);
	return intersection.lights[Direction_NorthSouth] == LightState_YELLOW;
}

bool north_south_is_red_after_its_yellow() {
	while(intersection.lights[Direction_NorthSouth] != LightState_YELLOW)
		next_state(&intersection);
	next_state(&intersection);
	return intersection.lights[Direction_NorthSouth] == LightState_RED;	
}


bool east_west_is_yellow_after_its_green() {
	while(intersection.lights[Direction_EastWest] != LightState_GREEN)
		next_state(&intersection);
	next_state(&intersection);
	return intersection.lights[Direction_EastWest] == LightState_YELLOW;
}

bool east_west_is_red_after_its_yellow() {
	while(intersection.lights[Direction_EastWest] != LightState_YELLOW)
		next_state(&intersection);
	next_state(&intersection);
	return intersection.lights[Direction_EastWest] == LightState_RED;
}

bool all_lights_red_after_north_south_yellow() {
	while(intersection.lights[Direction_NorthSouth] != LightState_YELLOW)
		next_state(&intersection);
	next_state(&intersection);
	return (intersection.lights[Direction_NorthSouth] == LightState_RED) && (intersection.lights[Direction_EastWest] == LightState_RED);
}


bool all_lights_red_after_east_west_yellow() {
	while(intersection.lights[Direction_EastWest] != LightState_YELLOW)
		next_state(&intersection);
	next_state(&intersection);
	return (intersection.lights[Direction_NorthSouth] == LightState_RED) && (intersection.lights[Direction_EastWest] == LightState_RED);
}

bool other_axis_turns_green_after_all_lights_red() {
	Direction last_active;
	//to make sure that last active is set correctly, you must cycle once to ensure it wasn't red on the first iteration
	do {
		last_active = intersection.active_direction;
		next_state(&intersection);
	} while(!all_lights_red());

	//now that all lights are red, advance the state machine and check if the OTHER light is green
	next_state(&intersection);
	return (intersection.active_direction != last_active) && (intersection.lights[intersection.active_direction] == LightState_GREEN);
}

/* Ensure that after many iterations, north-south and east-west never allow cars to pass at the same time */
bool at_least_one_axis_always_red() {
	for (int i = 0; i < 100000; ++i) {
		if((intersection.lights[Direction_NorthSouth] != LightState_RED) && (intersection.lights[Direction_EastWest] != LightState_RED)) {
			return false;
		}
	}

	return true;
}

void run_test_suite() {
	RUN_TEST(all_lights_set_to_stop_on_startup);
	RUN_TEST(initialize_resets_to_stop);
	RUN_TEST(north_south_is_yellow_after_its_green);
	RUN_TEST(north_south_is_red_after_its_yellow);
	RUN_TEST(east_west_is_yellow_after_its_green);
	RUN_TEST(east_west_is_red_after_its_yellow);
	RUN_TEST(all_lights_red_after_north_south_yellow);
	RUN_TEST(all_lights_red_after_east_west_yellow);
	RUN_TEST(other_axis_turns_green_after_all_lights_red);
	RUN_TEST(at_least_one_axis_always_red);
	printf("tests passed: %s%d/%d\033[0m\n", tests_passed == tests_run ? "\033[32m" : "\033[31m", tests_passed, tests_run);
}