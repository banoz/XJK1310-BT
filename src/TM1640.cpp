#include <Arduino.h>

#include <nrf_bitmask.h>
#include <nrf_gpio.h>
#include <nrf_gpiote.h>

#include "main.h"
#include "TM1640.h"

// #define PINMAP(x) g_ADigitalPinMap[x]
// #define PINMAP(x) x

uint32_t pinDIO = 0;
uint32_t pinSCLK = 0;
uint32_t pinBOTH = 0;

volatile bool buffered = false;
volatile bool dioState = false;

volatile uint16_t TM1640state = 0xFFFFU; // {counter}{data}

char TM1640Buffer[BUFFER_LENGTH] = {0};
volatile uint16_t TM1640BufferIndex = 0;

void IRQHandler(void);

void setupTM1640(uint32_t dio, uint32_t sclk, uint32_t both)
{
    attachInterrupt(dio, IRQHandler, CHANGE);
    attachInterrupt(sclk, IRQHandler, RISING);
    attachInterrupt(both, IRQHandler, FALLING);

    /*
    pinDIO = PINMAP(dio);
    pinSCLK = PINMAP(sclk);
    pinBOTH = PINMAP(both);

    nrf_gpio_cfg_input(pinDIO, NRF_GPIO_PIN_NOPULL);
    nrf_gpio_cfg_input(pinSCLK, NRF_GPIO_PIN_NOPULL);
    nrf_gpio_cfg_input(pinBOTH, NRF_GPIO_PIN_NOPULL);

    NVIC_DisableIRQ(GPIOTE_IRQn);
    NVIC_ClearPendingIRQ(GPIOTE_IRQn);

    nrf_gpiote_event_configure(NRF_GPIOTE, 0, pinDIO, NRF_GPIOTE_POLARITY_TOGGLE);
    nrf_gpiote_event_configure(NRF_GPIOTE, 1, pinSCLK, NRF_GPIOTE_POLARITY_LOTOHI);
    nrf_gpiote_event_configure(NRF_GPIOTE, 2, pinBOTH, NRF_GPIOTE_POLARITY_HITOLO);

    nrf_gpiote_int_enable(NRF_GPIOTE, GPIOTE_INTENSET_IN0_Set << GPIOTE_INTENSET_IN0_Pos);
    nrf_gpiote_int_enable(NRF_GPIOTE, GPIOTE_INTENSET_IN1_Set << GPIOTE_INTENSET_IN1_Pos);
    nrf_gpiote_int_enable(NRF_GPIOTE, GPIOTE_INTENSET_IN2_Set << GPIOTE_INTENSET_IN2_Pos);

    __NOP();
    __NOP();
    __NOP();

    NRF_GPIOTE->EVENTS_IN[0] = 0;
    NRF_GPIOTE->EVENTS_IN[1] = 0;
    NRF_GPIOTE->EVENTS_IN[2] = 0;

    nrf_gpiote_int_enable(NRF_GPIOTE, GPIOTE_INTENSET_IN0_Enabled << GPIOTE_INTENSET_IN0_Pos);
    nrf_gpiote_int_enable(NRF_GPIOTE, GPIOTE_INTENSET_IN1_Enabled << GPIOTE_INTENSET_IN1_Pos);
    nrf_gpiote_int_enable(NRF_GPIOTE, GPIOTE_INTENSET_IN2_Enabled << GPIOTE_INTENSET_IN2_Pos);

    nrf_gpiote_event_enable(NRF_GPIOTE, 0);
    nrf_gpiote_event_enable(NRF_GPIOTE, 1);
    nrf_gpiote_event_enable(NRF_GPIOTE, 2);

    NVIC_SetPriority(GPIOTE_IRQn, 6);
    NVIC_EnableIRQ(GPIOTE_IRQn);
    */
}

bool readTM1640(void)
{
    if (buffered)
    {
        parsePayload(TM1640Buffer);

        for (uint8_t i = 0; i < BUFFER_LENGTH; i++)
        {
            TM1640Buffer[i] = 0;
        }

        buffered = false;
        return true;
    }

    return false;
}

void pushBuffer(uint8_t data)
{
    if (data == 0x40)
    {
        if (TM1640BufferIndex <= 1)
        {
            return; // ignore first 0x40
        }
    }

    if (data == 0xC0)
    {
        TM1640BufferIndex = 1;
        return;
    }

    if (!buffered)
    {
        TM1640Buffer[TM1640BufferIndex++] = data;
        TM1640Buffer[0] = TM1640BufferIndex & 0xFFu; // set length in first byte

        if (TM1640BufferIndex >= BUFFER_LENGTH)
        {
            buffered = true;
            TM1640Buffer[0] = 0x00;
            TM1640BufferIndex = 1;
            TM1640state = 0xFFFFU;
        }
    }
}

inline void processEvent(uint8_t event)
{
    if (buffered)
    {
        TM1640state = 0xFFFFU;
        return; // ignore events until buffer is reset
    }

    if (event == 5) // start
    {
        TM1640state = 0;
        dioState = false;
        TM1640BufferIndex = 0;
        return;
    }

    if (TM1640state == 0xFFFFU)
    {
        return; // ignore events until start
    }

    if (event == 2) // CLK
    {
        uint8_t counter = TM1640state >> 8;

        if (counter >= 8)
        {
            pushBuffer(TM1640state & 0xFFu);

            TM1640state = 0;
            counter = 0;
        }

        if (dioState)
        {
            TM1640state |= 1U << counter;
        }

        TM1640state = (counter + 1U) << 8 | (TM1640state & 0xFFu);

        return;
    }

    if (event == 1) // DIO flip
    {
        dioState = !dioState;

        return;
    }

    if (event == 4)
    {
        return;
    }

    if (event == 7)
    {
        return;
    }
}

// extern "C" void GPIOTE_IRQHandler()
void IRQHandler(void)
{
    uint8_t event = 0;

    if (0 != NRF_GPIOTE->EVENTS_IN[0]) // pinDIO TOGGLE
    {
        event |= 1;
        NRF_GPIOTE->EVENTS_IN[0] = 0;
    }

    if (0 != NRF_GPIOTE->EVENTS_IN[1]) // pinSCLK LOTOHI
    {
        event |= 2;
        NRF_GPIOTE->EVENTS_IN[1] = 0;
    }

    if (0 != NRF_GPIOTE->EVENTS_IN[2]) // pinBOTH HITOLO
    {
        event |= 4;
        NRF_GPIOTE->EVENTS_IN[2] = 0;
    }

    if (event > 0)
    {
        processEvent(event);
    }
}

uint8_t mapSegment(uint8_t segmentValue)
{
    switch (segmentValue)
    {
    case 0x00:
    case 0x40:
    case 0x3F:
        return 0;
    case 0x06:
        return 1;
    case 0x5B:
        return 2;
    case 0x4F:
        return 3;
    case 0x66:
        return 4;
    case 0x6D:
        return 5;
    case 0x7D:
        return 6;
    case 0x07:
        return 7;
    case 0x7F:
        return 8;
    case 0x6F:
        return 9;
    default:
        return 0xFF; // invalid segment value
    }
}