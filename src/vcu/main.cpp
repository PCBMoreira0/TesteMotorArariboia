#include <Arduino.h>
#include "serialReader.hpp"
#include "canTasks.hpp"

#define HANDSHAKE_MCU_ID 0x1801D0EF
#define HANDSHAKE_VCU_ID 0x0C01EFD0
#define MESSAGE_VCU_ID 0x0C01EFD0

QueueHandle_t handshakeQueue = NULL;

bool handshakeComplete = false;
QueueHandle_t rpmQueue = NULL;

/// @brief Verifica e envia o RPM para a rpmQueue
/// @param parameter 
void rpmReadTask(void *parameter)
{
  rpmQueue = xQueueCreate(10, sizeof(int));
  if (rpmQueue == NULL)
    Serial.println("ERROR: rpmQueue não criada.");

  vTaskDelay(1000);

  const int maxRpm = 4500;
  while (true)
  {
    if (serialQueue == NULL)
    {
      vTaskDelay(pdMS_TO_TICKS(100));
      continue;
    }

    Serial.print("Digite o valor do novo RPM [0-5000]: ");

    char buffer[16];
    if (xQueueReceive(serialQueue, &buffer, portMAX_DELAY) != pdPASS)
    {
      Serial.println("ERROR: rpmReadTask falhou ao receber mensagem serial.");
    }

    int rpm;
    if (sscanf(buffer, "%d", &rpm) != 1)
    {
      Serial.println("ERROR: Não foi possível definir o RPM.");
      continue;
    }

    if (rpm < -maxRpm || rpm > maxRpm)
    {
      Serial.println("ERROR: RPM fora da escala [0-5000].");
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

/// @brief Codifica o RPM em uma mensagem CAN e envia para canTXQueue 
/// @param parameter 
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
        rpmToSend = -1 * rpm + 32000;
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
    message.data[4] = 0b00000001;
    message.data[5] = 0x0;
    message.data[6] = 0x0;
    message.data[7] = lifeCount++;

    if (xQueueSend(canTXQueue, &message, pdMS_TO_TICKS(10)) != pdPASS)
    {
      printf("ERROR: motorVCUTask falhou ao enviar mensagem para canTXQueue.\n");
    }

    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

/// @brief Estabelece a conexão com o MCU. 
/// @warning O handshake ocorre apenas uma vez, se falhar é necessário reiniciar a aplicação.
/// @param parameter 
void motorHandShake(void *parameter)
{
  handshakeQueue = xQueueCreate(10, sizeof(twai_message_t));
  if(handshakeQueue == NULL){
    Serial.println("ERROR: handshakeQueue não criada.");
  }
  handshakeComplete = false;
  Serial.println("Iniciando handshake...");
  vTaskDelay(pdMS_TO_TICKS(3000)); // 3 segundos para aguardar as mensagens do MCU estabilizarem
  Serial.println("Aguardando pedido do motor...");
  while (true)
  {
    twai_message_t message;

    if (xQueueReceive(handshakeQueue, &message, portMAX_DELAY) == pdPASS)
    {
      if (message.identifier == HANDSHAKE_MCU_ID)
      {
        Serial.println("Pedido de handshake recebido. Enviando resposta...");

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
          printf("ERROR: motorHandShake falhou ao enviar messagem para canTXQueue.\n");
          continue;
        }

        handshakeComplete = true;
        Serial.println("Handshake completo!");
        Serial.println("----------------------------");
        xTaskCreate(motorVCUTask, "VCU Task", 4096, NULL, 1, NULL);
        xTaskCreate(serialReadTask, "Serial Read", 4096, NULL, 1, NULL);
        xTaskCreate(rpmReadTask, "RPM Read", 4096, NULL, 1, NULL);
        vTaskDelete(NULL);
      }
    }
  }

  vTaskDelete(NULL);
}


/// @brief Envia uma cópia de mensagens CAN para outra tarefas
/// @param parameter 
void routerTask(void *parameter)
{
  while (true)
  {
    twai_message_t message;
    if (xQueueReceive(canRXQueue, &message, portMAX_DELAY) == pdPASS)
    {
      if (handshakeQueue != NULL && !handshakeComplete)
      {
        if (xQueueSend(handshakeQueue, &message, pdMS_TO_TICKS(1000)) != pdPASS)
        {
          Serial.println("ERROR: broker falhou ao enviar mensagem para handshakeQueue.");
        }
      }
    }

    // Insira outras filas aqui
  }
}

void setup()
{
  Serial.begin(115200);
  xTaskCreate(canTask, "Can Task", 4096, NULL, 1, NULL);
  xTaskCreate(motorHandShake, "Handshake", 4096, NULL, 1, NULL);
  xTaskCreate(routerTask, "Router Task", 4096, NULL, 1, NULL);
}

void loop()
{
  vTaskDelete(NULL);
}