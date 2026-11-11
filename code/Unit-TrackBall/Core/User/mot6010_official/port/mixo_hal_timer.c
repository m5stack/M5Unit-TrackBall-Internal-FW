#include "mixo_hal_timer.h"

#include "main.h"

static uint32_t mixo_hal_get_systick_us(void)
{
    uint32_t ticks_per_ms = SysTick->LOAD + 1U;
    uint32_t ms;
    uint32_t current_ticks;

    do {
        ms            = HAL_GetTick();
        current_ticks = SysTick->VAL;
    } while (ms != HAL_GetTick());

    if ((SCB->ICSR & SCB_ICSR_PENDSTSET_Msk) != 0U) {
        ms++;
        current_ticks = SysTick->VAL;
    }

    return (ms * 1000U) + (((ticks_per_ms - current_ticks) * 1000U) / ticks_per_ms);
}

void mixo_hal_delay_ms(uint32_t ms)
{
    HAL_Delay(ms);
}

void mixo_hal_delay_us(uint32_t us)
{
    uint32_t target_ticks;
    uint32_t previous_ticks;
    uint32_t current_ticks;
    uint32_t elapsed_ticks = 0U;
    uint32_t ticks_per_us  = SystemCoreClock / 1000000U;
    uint32_t reload        = SysTick->LOAD + 1U;

    if ((us == 0U) || (ticks_per_us == 0U)) {
        return;
    }

    target_ticks   = us * ticks_per_us;
    previous_ticks = SysTick->VAL;

    while (elapsed_ticks < target_ticks) {
        current_ticks = SysTick->VAL;
        if (current_ticks != previous_ticks) {
            if (current_ticks < previous_ticks) {
                elapsed_ticks += previous_ticks - current_ticks;
            } else {
                elapsed_ticks += reload - current_ticks + previous_ticks;
            }
            previous_ticks = current_ticks;
        }
    }
}

void mixo_hal_delay_nop(uint32_t count)
{
    while (count-- != 0U) {
        __NOP();
    }
}

uint32_t mixo_hal_get_tick_ms(void)
{
    return HAL_GetTick();
}

uint32_t mixo_hal_get_tick_us(void)
{
    return mixo_hal_get_systick_us();
}
