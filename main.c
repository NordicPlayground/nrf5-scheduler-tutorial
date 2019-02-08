/**
 * Copyright (c) 2014 - 2019, Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <stdbool.h>
#include <stdint.h>
#include "boards.h"
#include "nrf_drv_gpiote.h"
#include "app_error.h"
#include "app_timer.h"
#include "nrf_drv_clock.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"


APP_TIMER_DEF(m_led_a_timer_id);


/**@brief Button event handler.
 *
 * @details This function is responsible for starting or stopping toggling of LED 1 based on button
 *          presses.
 *
 *          Print a log line indicating weather executing in thread/main or interrupt handler mode.
 *
 */
static void button_handler(nrf_drv_gpiote_pin_t pin)
{
    uint32_t err_code;

    // Handle button press.
    switch (pin)
    {
    case BUTTON_1:
        NRF_LOG_INFO("Start toggling LED 1.");
        err_code = app_timer_start(m_led_a_timer_id, APP_TIMER_TICKS(500), NULL);
        APP_ERROR_CHECK(err_code);
        break;
    case BUTTON_2:
        NRF_LOG_INFO("Stop toggling LED 1.");
        err_code = app_timer_stop(m_led_a_timer_id);
        APP_ERROR_CHECK(err_code);
        break;
    default:
        break;
    }

    // Log execution mode.
    if (current_int_priority_get() == APP_IRQ_PRIORITY_THREAD)
    {
        NRF_LOG_INFO("Button handler is executing in thread/main mode.");
    }
    else
    {
        NRF_LOG_INFO("Button handler is executing in interrupt handler mode.");
    }
}


/**@brief Button event handler.
 */
static void gpiote_event_handler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
    // The button_handler function could be implemented here directly, but is extracted to a
    // separate function as it makes it easier to demonstrate the scheduler with less modifications
    // to the code later in the tutorial.

    button_handler(pin);
}


/**@brief Function for initializing GPIOs.
 */
static void gpio_init()
{
    ret_code_t err_code;

    // Initialze driver.
    err_code = nrf_drv_gpiote_init();
    APP_ERROR_CHECK(err_code);

    // Configure output pin for LED.
    nrf_drv_gpiote_out_config_t out_config = GPIOTE_CONFIG_OUT_SIMPLE(false);
    err_code = nrf_drv_gpiote_out_init(LED_1, &out_config);
    APP_ERROR_CHECK(err_code);

    // Set output pin to turn off LED (cathode is connected to the GPIO on the DK).
    nrf_drv_gpiote_out_set(LED_1);

    // Make a configuration for input pins. This is suitable for both pins in this example.
    nrf_drv_gpiote_in_config_t in_config = GPIOTE_CONFIG_IN_SENSE_HITOLO(true);
    in_config.pull = NRF_GPIO_PIN_PULLUP;

    // Configure input pins for buttons, with separate event handlers for each button.
    err_code = nrf_drv_gpiote_in_init(BUTTON_1, &in_config, gpiote_event_handler);
    APP_ERROR_CHECK(err_code);
    err_code = nrf_drv_gpiote_in_init(BUTTON_2, &in_config, gpiote_event_handler);
    APP_ERROR_CHECK(err_code);

    // Enable input pins for buttons.
    nrf_drv_gpiote_in_event_enable(BUTTON_1, true);
    nrf_drv_gpiote_in_event_enable(BUTTON_2, true);
}


/**@brief Timeout handler for the repeated timer used for toggling LED 1.
 *
 * @details Print a log line indicating weather executing in thread/main or interrupt mode.
 */
static void timer_handler(void * p_context)
{
    // Toggle LED.
    nrf_drv_gpiote_out_toggle(LED_1);

    // Log execution mode.
    if (current_int_priority_get() == APP_IRQ_PRIORITY_THREAD)
    {
        NRF_LOG_INFO("Timeout handler is executing in thread/main mode.");
    }
    else
    {
        NRF_LOG_INFO("Timeout handler is executing in interrupt handler mode.");
    }
}


/**@brief Create timers.
 */
static void timer_init()
{
    uint32_t err_code;

    err_code = app_timer_init();
    APP_ERROR_CHECK(err_code);

    err_code = app_timer_create(&m_led_a_timer_id,
                                APP_TIMER_MODE_REPEATED,
                                timer_handler);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function starting the internal LFCLK oscillator.
 *
 * @details This is needed by RTC1 which is used by the Application Timer
 *          (When SoftDevice is enabled the LFCLK is always running and this is not needed).
 */
static void lfclk_request(void)
{
    uint32_t err_code = nrf_drv_clock_init();
    APP_ERROR_CHECK(err_code);
    nrf_drv_clock_lfclk_request(NULL);
}


/**@brief Function for initializing logging.
 */
static void log_init(void)
{
    ret_code_t err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_DEFAULT_BACKENDS_INIT();
}


/**@brief Main function.
 */
int main(void)
{
    lfclk_request();
    log_init();
    gpio_init();
    timer_init();

    NRF_LOG_INFO("Scheduler tutorial example started.");

    // Enter main loop.
    while (true)
    {
        __WFI();
    }
}
