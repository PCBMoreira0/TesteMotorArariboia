#include "canTasks.hpp"

QueueHandle_t canRXQueue = NULL;
QueueHandle_t canTXQueue = NULL;

static void initialize_can_driver()
{
    // Initialize configuration structures using macro initializers
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(GPIO_NUM_16, GPIO_NUM_17, TWAI_MODE_NORMAL);
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_250KBITS();
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    // Install TWAI driver
    if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK)
    {
        Serial.printf("Driver installed\n");
    }
    else
    {
        Serial.printf("Failed to install driver\n");
        return;
    }

    // Start TWAI driver
    if (twai_start() == ESP_OK)
    {
        printf("Driver started\n");
    }
    else
    {
        printf("Failed to start driver\n");
        return;
    }
}

void canReceiveTask(void *parameter)
{
    while (true)
    {
        // Wait for message to be received
        twai_message_t message;
        if (twai_receive(&message, portMAX_DELAY) != ESP_OK)
        {
            printf("Failed to receive message\n");
            continue;
        }

        if (xQueueSend(canRXQueue, &message, pdMS_TO_TICKS(1000)) != pdPASS)
        {
            printf("CAN TASK: Failed to send queue.");
        }
    }
}

void canTransmitTask(void *parameter)
{
    while (true)
    {
        twai_message_t message;
        if (xQueueReceive(canTXQueue, &message, pdMS_TO_TICKS(portMAX_DELAY)) == pdPASS)
        {
            if (twai_transmit(&message, pdMS_TO_TICKS(1000)) != ESP_OK)
            {
                printf("Failed to queue message for transmission\n");
            }
        }
    }
}

void canTask(void *parameter)
{
    canRXQueue = xQueueCreate(10, sizeof(twai_message_t));
    if (canRXQueue == NULL)
    {
        Serial.println("ERROR: CAN RX falhou ao criar a fila.");
    }
    canTXQueue = xQueueCreate(10, sizeof(twai_message_t));
    if (canTXQueue == NULL)
    {
        Serial.println("ERROR: CAN TX falhou ao criar a fila.");
    }

    initialize_can_driver();
    xTaskCreate(canReceiveTask, "Can Receive", 4096, NULL, 1, NULL);
    xTaskCreate(canTransmitTask, "Can Transmit", 4096, NULL, 1, NULL);

    vTaskDelete(NULL);
}
