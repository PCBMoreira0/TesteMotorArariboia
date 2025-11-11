#include <Arduino.h>
#include "serialReader.hpp"

QueueHandle_t serialQueue = NULL;

void initializeQueue(){

}

void clearSerialBuffer()
{
    while (Serial.available())
    {
        Serial.read();
    }
}

void serialReadTask(void *parameter)
{
    serialQueue = xQueueCreate(1, 16 * sizeof(char));
    if (serialQueue == NULL)
        Serial.println("ERROR: Erro ao criar a fila.");

    while (true)
    {
        clearSerialBuffer();

        while (!Serial.available())
        {
            vTaskDelay(pdMS_TO_TICKS(100));
        }

        uint8_t index = 0;
        const uint8_t bufferSize = 16;
        char buffer[bufferSize];

        char c = Serial.read();
        while (c != '\n' && c != '\r' && index < bufferSize - 1)
        {
            if (c == '\b')
            {
                index--;
                Serial.print("\b ");
            }
            else
            {
                buffer[index++] = c;
            }

            Serial.print(c);

            while (!Serial.available())
            {
                vTaskDelay(100);
            }
            c = Serial.read();
        }
        Serial.println();

        buffer[index++] = '\0';

        if (serialQueue != NULL)
        {
            if (xQueueSend(serialQueue, &buffer, portMAX_DELAY) != pdPASS)
            {
                Serial.println("ERROR: Erro ao enviar mensagem serial.");
            }
        }
        else
        {
            Serial.println("ERROR: Fila nula.");
        }
    }

    vTaskDelete(NULL);
}