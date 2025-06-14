#include <stdio.h>
#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "led.h"
#include "buzzer.h"
#include "button.h"
#include "hardware.h"

#define STACK_SIZE 256

typedef void (*Multitask)(void*);

TaskHandle_t blink_handle;
TaskHandle_t buzzer_handle;
TaskHandle_t button_handle;


void vBlinkingLights( void * pvParameters )
{
    configASSERT( ( ( uint32_t ) pvParameters ) == 1 );

    for(int light_selector = 0 ;; light_selector++)
    {
        light_selector %= 3;
        int LIGHT_PIN = GREEN + light_selector;

        gpio_put(LIGHT_PIN, true);
        vTaskDelay(pdMS_TO_TICKS(500));
        gpio_put(LIGHT_PIN, false);
    }
}

void vBuzzerBeep( void * pvParameters )
{
    configASSERT( ( ( uint32_t ) pvParameters ) == 1 );

    for( ;; )
    {
        gpio_put(BUZZER_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(70));

        gpio_put(BUZZER_PIN, 0);
        vTaskDelay(pdMS_TO_TICKS(930));
    }
}

void vButtonControl( void * pvParameters )
{
    configASSERT( ( ( uint32_t ) pvParameters ) == 1 );

    bool isBlinkSuspended = false;
    bool isBuzzerSuspended = false;

    for( ;; )
    {

        if(!gpio_get(A))
        {
            if(isBlinkSuspended)
                vTaskResume(blink_handle);
            else
                vTaskSuspend(blink_handle);

            isBlinkSuspended = !isBlinkSuspended;
            vTaskDelay(pdMS_TO_TICKS(150));
        }

        if(!gpio_get(B))
        {
            if(isBuzzerSuspended)
                vTaskResume(buzzer_handle);
            else
                vTaskSuspend(buzzer_handle);

            isBuzzerSuspended = !isBuzzerSuspended;
            vTaskDelay(pdMS_TO_TICKS(150));
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

TaskHandle_t vCreateFunction(char * name, Multitask func)
{
    BaseType_t xReturned;
    TaskHandle_t xHandle = NULL;

    xReturned = xTaskCreate(
                    func,      
                    name,         
                    STACK_SIZE,     
                    ( void * ) 1,    
                    tskIDLE_PRIORITY,
                    &xHandle );     

    return xHandle;
}

int main()
{
    stdio_init_all();
    boot_hardware();

    blink_handle = vCreateFunction("blink", vBlinkingLights);
    buzzer_handle = vCreateFunction("buzzer", vBuzzerBeep);
    button_handle = vCreateFunction("button_control", vButtonControl);

    vTaskStartScheduler();

    while (true);

    return 0;
}
