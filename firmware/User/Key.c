#include "Config.h"
#include "include.h"


void Key_Init(void) {
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

    GPIO_StructInit(&GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = KEY1_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(KEY1_GPIO, &GPIO_InitStructure);

}


uint8_t Key_Read(void) {
    static uint8_t raw_state = Bit_SET;
    static uint8_t stable_state = Bit_SET;
    static uint32_t changed_ms;
    uint8_t sample = GPIO_ReadInputDataBit(KEY1_GPIO, KEY1_PIN);
    uint32_t now = Timer_GetMs();

    if (sample != raw_state) {
        raw_state = sample;
        changed_ms = now;
    }

    if (stable_state != raw_state && (uint32_t)(now - changed_ms) >= 20U) {
        stable_state = raw_state;
        if (stable_state == Bit_RESET) {
            return 1U;
        }
    }

    return 0U;
}
