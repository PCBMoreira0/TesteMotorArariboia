#include <Arduino.h>
#include "canTasks.hpp"

void sniffer_task(void *parameter){
  while(true){
    twai_message_t message;
    if(xQueueReceive(canRXQueue, &message, pdMS_TO_TICKS(portMAX_DELAY)) == pdPASS){
      Serial.printf("%X  ", message.identifier);
      for(int i = 0; i < 8; i++){
        Serial.printf("%02X ", message.data[i]);
      }
      Serial.println();
    }
  }
}

void setup()
{
  Serial.begin(115200);
  xTaskCreate(canTask, "Can Task", 4096, NULL, 1, NULL);
  xTaskCreate(sniffer_task, "Sniffer Task", 4096, NULL, 1, NULL);
}

void loop()
{
  vTaskDelete(NULL);
}