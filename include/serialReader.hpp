#pragma once
#include <Arduino.h>

extern QueueHandle_t serialQueue;

void serialReadTask(void *parameter);