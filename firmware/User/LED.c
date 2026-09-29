#include "Config.h"
#include "include.h"

void LED_Init(void){
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC | RCC_AHB1Periph_GPIOB, ENABLE);

    GPIO_StructInit(&GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    GPIO_StructInit(&GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    LED_OFF(LED1_GPIO, LED1_PIN);
    LED_OFF(LED2_GPIO, LED2_PIN);
}


void LED_OFF(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin) {
    GPIO_SetBits(GPIOx, GPIO_Pin);
}


void LED_ON(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin) {
    GPIO_ResetBits(GPIOx, GPIO_Pin);
}


void LED_T(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin) {
    if (GPIO_ReadInputDataBit(GPIOx, GPIO_Pin)) {
        LED_ON(GPIOx, GPIO_Pin);
    }
    else {
        LED_OFF(GPIOx, GPIO_Pin);
    }
}
