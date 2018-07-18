//
// Created by Oleksandra Baga on 12.05.18.
//
#include <stdlib.h>
#include <time.h>

int battery_level = 100;

int get_battery_level() {

    // RANDOM BATTERY LEVEL GENERATION FOR TEST PURPOSE

    srand(time(NULL));

    battery_level = 80+rand()%20;

    return battery_level;
}


