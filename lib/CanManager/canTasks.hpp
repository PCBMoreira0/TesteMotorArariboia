#pragma once
#include <Arduino.h>
#include <driver/twai.h>

extern QueueHandle_t canRXQueue;
extern QueueHandle_t canTXQueue;
void can_task(void *parameter);