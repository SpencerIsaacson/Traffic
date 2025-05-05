#ifndef TRAFFIC_H
#define TRAFFIC_H

/* 
	ZII (zero-is-initialization) red is the 0 state for safety, 
	so the intersection starts with all lights red
*/
typedef enum {
	LightState_RED,
	LightState_YELLOW,
	LightState_GREEN,
	LightState_count,
} LightState;

typedef enum {
	Direction_NorthSouth,
	Direction_EastWest,
	Direction_count
} Direction;

/*
	We could have a separate value for every light, but since there are no protected turns at this intersection, 
	it's more reliable to have lights that are on a shared axis	share a signal so there's no possibility of them
	getting out of sync with one another. So east-west share a single signal, and north-south share a signal.
*/
typedef struct {
	LightState lights[Direction_count];
	Direction active_direction;
} Intersection;

bool inited;
Intersection intersection;

/* This is technically not needed on program startup, as globally scoped items have static storage,
and are automatically zeroed out, but it's useful for reseting the intersection at runtime.*/
void init_intersection(Intersection *intersection) {
	*intersection = (Intersection){};
	inited = true; //has the intersection *ever* been reset since startup?
}

#define active_light (intersection->lights[intersection->active_direction])

void next_state(Intersection *intersection) {
	switch(active_light) {
		case LightState_RED: {
			active_light = LightState_GREEN;
		} break;
		case LightState_GREEN: {
			active_light = LightState_YELLOW;
		} break;
		case LightState_YELLOW: {
			active_light = LightState_RED;
			intersection->active_direction = (intersection->active_direction + 1) % Direction_count;
		} break;		
	}
}

//interval of each stage in milliseconds
unsigned int intervals[LightState_count] = {
	[LightState_GREEN]  = 5000,
	[LightState_YELLOW] = 1000,
	[LightState_RED]    = 2500,
};

clock_t previous_clock;

void basic_time_strategy(Intersection *intersection) {
	clock_t current_clock = clock();
	if(((current_clock - previous_clock) * 1000.0 / CLOCKS_PER_SEC) > intervals[active_light]) {
		previous_clock = current_clock;
		next_state(intersection);
	}
}

#undef active_light

/*
	This is designed to allow extension, for instance if one were to implement a system 
	where weight-based sensors detect traffic at the intersection and adjust the urgency
	of switching the light accordingly, you could swap the function pointer for the new strategy.
	As it stands it just changes the lights at pre-determined intervals
*/
typedef void FunctionType(Intersection *intersection);
FunctionType *traffic_strategy = basic_time_strategy;

#endif