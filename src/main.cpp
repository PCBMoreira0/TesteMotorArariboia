#include <Arduino.h>
#include "serialReader.hpp"

void rpmReadTask(void *parameter)
{
  QueueHandle_t queue = *((QueueHandle_t *)parameter);
  const int maxRpm = 4500;
  while (true)
  {
    if(queue == NULL) continue;

    Serial.print("Digite o valor do novo RPM [0-4500]: ");

    char buffer[16];
    if (xQueueReceive(queue, &buffer, portMAX_DELAY) != pdPASS)
    {
      Serial.println("ERROR: Erro ao receber mensagem serial | rpmReadTask");
    }

    int rpm;
    if (sscanf(buffer, "%d", &rpm) != 1)
    {
      continue;
    }

    if (rpm < -maxRpm || rpm > maxRpm)
    {
      Serial.println("RPM fora da escala [0-4500].");
      continue;
    }

    Serial.printf("RPM definido para %d.\n\n", rpm);
  }

  vTaskDelete(NULL);
}

void setup()
{
  Serial.begin(115200);

  xTaskCreate(serialReadTask, "Serial Read", 4096, NULL, 1, NULL);
  xTaskCreate(rpmReadTask, "RPM Read", 4096, &serialQueue, 1, NULL);
}

void loop()
{
  vTaskDelete(NULL);
}