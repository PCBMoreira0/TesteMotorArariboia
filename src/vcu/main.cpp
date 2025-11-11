#include <Arduino.h>
#include "serialReader.hpp"
#include "canTasks.hpp"

#define HANDSHAKE_MCU_ID 0x1801D0EF
#define HANDSHAKE_VCU_ID 0x0C01EFD0
#define MESSAGE_VCU_ID 0x0C01EFD0

QueueHandle_t rpmQueue = NULL;

void rpmReadTask(void *parameter)
{
  rpmQueue = xQueueCreate(10, sizeof(int));
  if (rpmQueue == NULL)
    Serial.println("ERROR: Fila RPM não criada.");

  vTaskDelay(1000);

  const int maxRpm = 4500;
  while (true)
  {
    if (serialQueue == NULL)
    {
      vTaskDelay(pdMS_TO_TICKS(100));
      continue;
    }

    Serial.print("Digite o valor do novo RPM [0-4500]: ");

    char buffer[16];
    if (xQueueReceive(serialQueue, &buffer, portMAX_DELAY) != pdPASS)
    {
      Serial.println("ERROR: Erro ao receber mensagem serial | rpmReadTask");
    }

    int rpm;
    if (sscanf(buffer, "%d", &rpm) != 1)
    {
      Serial.println("ERROR: Não foi possível definir o RPM.");
      continue;
    }

    if (rpm < -maxRpm || rpm > maxRpm)
    {
      Serial.println("ERROR: RPM fora da escala [0-4500].");
      continue;
    }

    if (xQueueSend(rpmQueue, &rpm, portMAX_DELAY) == pdPASS)
    {
      Serial.printf("RPM definido para %d.\n\n", rpm);
    }
    else
    {
      Serial.println("ERROR: Falhou ao enviar rpm para a fila.");
    }
  }
}

void motorVCUTask(void *parameter)
{
  uint16_t rpmToSend = 32000;
  uint8_t lifeCount = 0;
  while (true)
  {
    int rpm;
    if (rpmQueue != NULL)
    {
      if (xQueueReceive(rpmQueue, &rpm, 0) == pdPASS)
      {
        rpmToSend = rpm + 32000;
      }
    }

    twai_message_t message;
    message.identifier = MESSAGE_VCU_ID;
    message.extd = TWAI_MSG_FLAG_EXTD;
    message.data_length_code = 8;
    message.data[0] = 0x0;
    message.data[1] = 0x0;
    message.data[2] = (uint8_t)rpmToSend;
    message.data[3] = (uint8_t)(rpmToSend >> 8);
    message.data[4] = 0b11000000;
    message.data[5] = 0x0;
    message.data[6] = 0x0;
    message.data[7] = lifeCount++;

    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

void motorHandShake(void *parameter)
{
  while (true)
  {
    twai_message_t message;
    if (xQueueReceive(canRXQueue, &message, portMAX_DELAY) == pdPASS)
    {
      if (message.identifier == HANDSHAKE_MCU_ID)
      {
        twai_message_t message;
        message.identifier = HANDSHAKE_VCU_ID;
        message.extd = TWAI_MSG_FLAG_EXTD;
        message.data_length_code = 8;
        for (int i = 0; i < 8; i++)
        {
          message.data[i] = 0xAA;
        }

        if (xQueueSend(canTXQueue, &message, pdMS_TO_TICKS(1000)) != pdPASS)
        {
          printf("Failed to queue message for transmission\n");
        }
      }
    }
  }
}

void setup()
{
  Serial.begin(115200);
  xTaskCreate(can_task, "Can Task", 4096, NULL, 1, NULL);
  xTaskCreate(serialReadTask, "Serial Read", 4096, NULL, 1, NULL);
  xTaskCreate(rpmReadTask, "RPM Read", 4096, NULL, 1, NULL);
  xTaskCreate(motorHandShake, "Handshake", 4096, NULL, 1, NULL);
  xTaskCreate(motorVCUTask, "VCU Task", 4096, NULL, 1, NULL);
}

void loop()
{
  vTaskDelete(NULL);
}