/*
 * main.cpp
 *
 *  Created on: Feb 27, 2017
 *      Author: Zarnowski
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>
#include <signal.h>
#include <letmecreate/letmecreate.h>
#include <sys/time.h>
#include <pthread.h>

enum DoorAction {
    DoorAction_NONE,
    DoorAction_OPENING,
    DoorAction_CLOSING,
};

enum Limiter {
    Limiter_NONE,
    Limiter_UPPER,
    Limiter_LOWER
};

static int g_Speed_slow_open = 50;
static int g_Speed_slow_close = 70;

static int g_StartStopButtonMonitorId = 0;
static int g_SpeedButtonMonitorId = 0;
static bool g_MainLoopRunning = false;
static int g_StartStopButtonGPIO = 0;
static int g_SpeedButtonGPIO = 0;
static int g_UpperLimiterGPIO = 0;
static int g_LowerLimiterGPIO = 0;
static DoorAction g_LastDoorAction = DoorAction_NONE;
static Limiter g_LimiterToIgnore = Limiter_NONE;
static bool g_StartStopButtonStateChanged = false;
static bool g_SpeedButtonStateChanged = false;
static bool g_ButtonsStateChangeHandled = true;
static pthread_mutex_t g_MasterMutex;
static bool g_DoorInMovement = false;
static bool g_SlowSpeed = false;

void logLine(const char* text) {
    fprintf(stdout, "%s\n", text);
    fflush(stdout);
}

void startOpenDoor() {
    logLine(g_SlowSpeed ? "Opening doors slow." : "Opening doors fast.");
    dc_motor_click_set_direction(MIKROBUS_1, 1);
    dc_motor_click_set_speed(MIKROBUS_1, g_SlowSpeed ? g_Speed_slow_open : 100.0f);
    g_DoorInMovement = true;
    g_LimiterToIgnore = Limiter_LOWER;
}

void startCloseDoor() {
    logLine(g_SlowSpeed ? "Closing doors slow." : "Closing doors fast.");
    dc_motor_click_set_direction(MIKROBUS_1, 0);
    dc_motor_click_set_speed(MIKROBUS_1, g_SlowSpeed ? g_Speed_slow_close : 100.0f);
    g_DoorInMovement = true;
    g_LimiterToIgnore = Limiter_UPPER;
}

void stopDoors() {
    logLine("Stop door movement.");
    dc_motor_click_set_speed(MIKROBUS_1, 0);
    g_DoorInMovement = false;
    g_LimiterToIgnore = Limiter_NONE;
}

void startStopButtonStateChanged(uint8_t eventType) {
    pthread_mutex_lock(&g_MasterMutex);
    if (g_ButtonsStateChangeHandled == false) {
        pthread_mutex_unlock(&g_MasterMutex);
        return;
    }
    g_StartStopButtonStateChanged = true;
    g_ButtonsStateChangeHandled = false;
    pthread_mutex_unlock(&g_MasterMutex);
}

void speedButtonStateChanged(uint8_t eventType) {
    pthread_mutex_lock(&g_MasterMutex);
    if (g_ButtonsStateChangeHandled == false) {
        pthread_mutex_unlock(&g_MasterMutex);
        return;
    }
    g_SpeedButtonStateChanged = true;
    g_ButtonsStateChangeHandled = false;
    pthread_mutex_unlock(&g_MasterMutex);
}

bool setupStartStopButtonGPIO(int gpio) {
    if (gpio_init(gpio) < 0) {
        logLine("[Setup] Can't acquire gpio for 'Start/Stop' button.");
        return false;
    }
    if (gpio_set_direction(gpio, GPIO_INPUT) < 0) {
        logLine("[Setup] Can't set direction for 'Start/Stop' button gpio.");
        return false;
    }
    gpio_monitor_init();
    g_StartStopButtonMonitorId = gpio_monitor_add_callback(gpio, GPIO_FALLING, &startStopButtonStateChanged);
    if (g_StartStopButtonMonitorId < 0) {
        logLine("[Setup] Can't set observation for 'Start/Stop' button gpio.");
        return false;
    }
    g_StartStopButtonGPIO = gpio;
    logLine("[Setup] 'Start/Stop' button observation started.");
    return true;
}

bool setupSpeedButtonGPIO(int gpio) {
    if (gpio_init(gpio) < 0) {
        logLine("[Setup] Can't acquire gpio for 'Speed' button.");
        return false;
    }
    if (gpio_set_direction(gpio, GPIO_INPUT) < 0) {
        logLine("[Setup] Can't set direction for 'Speed' button gpio.");
        return false;
    }
    gpio_monitor_init();
    g_SpeedButtonMonitorId = gpio_monitor_add_callback(gpio, GPIO_FALLING, &speedButtonStateChanged);
    if (g_SpeedButtonMonitorId < 0) {
        logLine("[Setup] Can't set observation for 'Speed' button gpio.");
        return false;
    }
    g_SpeedButtonGPIO = gpio;
    logLine("[Setup] 'Speed' button observation started.");
    return true;
}

void tearDownButtons() {
    if (g_StartStopButtonMonitorId != 0) {
        gpio_monitor_remove_callback(g_StartStopButtonMonitorId);
    }
    if (g_SpeedButtonMonitorId != 0) {
        gpio_monitor_remove_callback(g_SpeedButtonMonitorId);
    }
    gpio_monitor_release();
    gpio_release(g_StartStopButtonGPIO);
    gpio_release(g_SpeedButtonGPIO);
    logLine("[TearDown] Stopped observation of buttons.");
}

bool setupLimiters(int lowerDoorPin, int upperDoorPin) {
    if (gpio_init(lowerDoorPin) < 0) {
        logLine("[Setup] Can't init gpio for lower limiter.");
        return false;
    }
    if (gpio_set_direction(lowerDoorPin, GPIO_INPUT) < 0) {
        logLine("[Setup] Can't set direction for gpio for lower limiter.");
        return false;
    }
    g_LowerLimiterGPIO = lowerDoorPin;

    if (gpio_init(upperDoorPin) < 0) {
        logLine("[Setup] Can't init gpio for upper limiter.");
        return false;
    }
    if (gpio_set_direction(upperDoorPin, GPIO_INPUT) < 0) {
        logLine("[Setup] Can't set direction for gpio for upper limiter.");
        return false;
    }
    g_UpperLimiterGPIO = upperDoorPin;

    logLine("[Setup] Acquired gpio for upper and lower limiters.");
    return true;
}

void tearDownLimiters() {
    if (g_LowerLimiterGPIO != 0) {
        gpio_release(g_LowerLimiterGPIO);
        g_LowerLimiterGPIO = 0;
        logLine("[TearDown] Released gpio for lower limiter.");
    }

    if (g_UpperLimiterGPIO != 0) {
        gpio_release(g_UpperLimiterGPIO);
        g_UpperLimiterGPIO = 0;
        logLine("[TearDown] Released gpio for upper limiter.");
    }
}

Limiter getCurrentLimiter() {
    uint8_t upper, lower;
    gpio_get_value(g_LowerLimiterGPIO, &lower);
    gpio_get_value(g_UpperLimiterGPIO, &upper);
    //printf("UP:%d, DOWN:%d ", upper,lower);
    Limiter current;
    if (upper == 1 && lower == 0) {
        current = Limiter_LOWER;
        printf(" - LOWER\n");

    } else if (upper == 0 && lower == 1) {
        current = Limiter_UPPER;
        printf(" - UPPER\n");

    } else {
        current = Limiter_NONE;
        printf(" - NONE\n");
    }
    return current;
}

void detectLastAction() {
    Limiter cur = getCurrentLimiter();
    if (cur == Limiter_LOWER) {
        g_LastDoorAction = DoorAction_CLOSING;
        logLine("[Setup] Doors are close, next action will be OPEN.");

    } else if (cur == Limiter_UPPER) {
        g_LastDoorAction = DoorAction_OPENING;
        logLine("[Setup] Doors are close, next action will be OPEN.");

    } else {
        logLine("[Setup] No limiters are triggered, can't determine door position. Next action will be OPEN.");
        g_LastDoorAction = DoorAction_OPENING;
    }
}

bool initMasterMutex() {
    if (pthread_mutex_init(&g_MasterMutex, NULL) != 0) {
        logLine("[Setup] I can't init mutex, this is bad and I'm sad :( \n");
        return false;
    }
    logLine("[Setup] Master mutex is ready to serve.");
    return true;
}

void cleanup() {
    tearDownButtons();
    tearDownLimiters();
    dc_motor_click_release(MIKROBUS_1);

    pthread_mutex_destroy(&g_MasterMutex);
    logLine("[TearDown] Master mutex is destroyed.");
}

void changeDoorDirection() {
    //NOTE: Called inside critical section!
    if (g_LastDoorAction == DoorAction_CLOSING) {
        g_LastDoorAction = DoorAction_OPENING;
        startOpenDoor();

    } else {
        g_LastDoorAction = DoorAction_CLOSING;
        startCloseDoor();
    }
}

void checkForLimitersTrigger() {
    if (g_DoorInMovement == false) {
        return;
    }
    Limiter current = getCurrentLimiter();

    if (current != Limiter_NONE && g_LimiterToIgnore != current) {
        stopDoors();
    }
}

void changeDoorSpeed() {
    g_SlowSpeed = !g_SlowSpeed;
    logLine(g_SlowSpeed ? "Switching speed to slow" : "Switching speed to fast");
}

static int ParseCommandArgs(int argc, char *argv[]) {
    int opt;
    opterr = 0;

    while (1) {
        opt = getopt(argc, argv, "o:c:h");
        if (opt == -1) break;

        switch (opt) {
            case 'o':
                g_Speed_slow_open = (unsigned int)strtoul(optarg, NULL, 0);
                g_Speed_slow_open = g_Speed_slow_open > 100 ? 100 : g_Speed_slow_open;
                g_Speed_slow_open = g_Speed_slow_open < 10 ? 10 : g_Speed_slow_open;
                break;

            case 'c':
                g_Speed_slow_close = (unsigned int)strtoul(optarg, NULL, 0);
                g_Speed_slow_close = g_Speed_slow_close > 100 ? 100 : g_Speed_slow_close;
                g_Speed_slow_close = g_Speed_slow_close < 10 ? 10 : g_Speed_slow_close;
                break;

            case 'h':
            default:
                printf("Usage:\n");
                printf("-o prc - set percent speed for slow open (0-100), default:%d\n", g_Speed_slow_open);
                printf("-c prc - set percent speed for slow close (0-100), default:%d\n", g_Speed_slow_close);
                return -1;
        }
    }
    return 1;
}

static void ChangeSpeedCallback(void) {
    speedButtonStateChanged(0);
}

static void StartStopButtonCallback(void) {
    startStopButtonStateChanged(0);
}

void controls_Init() {
    printf("[Setup] Enabling onboard button controls.\n");
    bool result = switch_init() == 0;
    result &= switch_add_callback(SWITCH_2_PRESSED, ChangeSpeedCallback) == 0;
    result &= switch_add_callback(SWITCH_1_PRESSED, StartStopButtonCallback) == 0;
    if (result == false) {
        printf("[Setup] Problems while acquiring buttons, local provision control might not work.\n");
    }
}

int main(int argc, char **argv) {
    //NOTE: Control jumper JP1 & JP9 in position 2-3
    if (initMasterMutex() == false) {
        return -1;
    }

    ParseCommandArgs(argc, argv);
    controls_Init();
    printf("[Setup] Using slow close speed: %d\n", g_Speed_slow_close);
    printf("[Setup] Using slow open speed: %d\n", g_Speed_slow_open);

    bool result = dc_motor_click_init(MIKROBUS_1) == 0;
    result &= setupStartStopButtonGPIO(MIKROBUS_2_RST);
    result &= setupSpeedButtonGPIO(MIKROBUS_2_AN);
    result &= setupLimiters(MIKROBUS_2_INT, MIKROBUS_2_PWM);

    if (result == false) {
        logLine("Can't acquire needed GPIO pins, leaving\n");
        cleanup();
        return -1;
    }
    detectLastAction();

    g_MainLoopRunning = true;
    stopDoors();
    while(g_MainLoopRunning) {

        pthread_mutex_lock(&g_MasterMutex);
        if (g_StartStopButtonStateChanged == true) {
            if (g_DoorInMovement) {
                stopDoors();
            } else {
                changeDoorDirection();
            }
            g_ButtonsStateChangeHandled = true;
            g_StartStopButtonStateChanged = false;
        }
        if (g_SpeedButtonStateChanged == true) {
            changeDoorSpeed();
            g_ButtonsStateChangeHandled = true;
            g_SpeedButtonStateChanged = false;
        }
        pthread_mutex_unlock(&g_MasterMutex);

        checkForLimitersTrigger();
        usleep(250000);
    }

    cleanup();
}
