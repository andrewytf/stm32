#include <stddef.h>
#include <string.h>

#include "ch.h"
#include "hal.h"
#include "utils_general.h"
#include "utils_hal.h"

#include "RNHPort.h"

typedef enum {
    RNH_PORT_STATUS = 0,
    RNH_PORT_FAULT = 1,
    RNH_PORT_ON = 2,
    RNH_PORT_OFF = 3,
    RNH_PORT_CURRENT_FREQ = 4
} RNHAction;

static void cmd_port(struct RCICmdData * rci_data, void * user_data UNUSED);
const struct RCICommand RCI_CMD_PORT = {
    .name = "#PORT",
    .function = cmd_port,
    .user_data = NULL
};

#define NUM_PORT 8
static const uint32_t power[NUM_PORT] = {
    GPIO_E0_NODE1_N_EN,
    GPIO_E1_NODE2_N_EN,
    GPIO_E2_NODE3_N_EN,
    GPIO_E3_NODE4_N_EN,
    GPIO_E4_NC,
    GPIO_E5_NODE6_N_EN,
    GPIO_E6_NODE7_N_EN,
    GPIO_E7_NODE8_N_EN
};
static const uint32_t fault[NUM_PORT] = {
    GPIO_E8_NODE1_N_FLT,
    GPIO_E9_NODE2_N_FLT,
    GPIO_E10_NODE3_N_FLT,
    GPIO_E11_NODE4_N_FLT,
    GPIO_E12_NC,
    GPIO_E13_NODE6_N_FLT,
    GPIO_E14_NODE7_N_FLT,
    GPIO_E15_NODE8_N_FLT
};

EVENTSOURCE_DECL(rnhPortCurrent);
static uint16_t outBuffer[8];
static int activeport;

static void select_port_imon(int port){
    switch(port){
    case 0:
        palClearPad(GPIOD, GPIO_D7_IMON_A0);
        palClearPad(GPIOD, GPIO_D8_IMON_A1);
        break;
    case 1:
        palSetPad(GPIOD, GPIO_D7_IMON_A0);
        palClearPad(GPIOD, GPIO_D8_IMON_A1);
        break;
    case 2:
        palClearPad(GPIOD, GPIO_D7_IMON_A0);
        palSetPad(GPIOD, GPIO_D8_IMON_A1);
        break;
    case 3:
        palSetPad(GPIOD, GPIO_D7_IMON_A0);
        palSetPad(GPIOD, GPIO_D8_IMON_A1);
        break;
    }
}

static void ADCCallback(ADCDriver *adcp, adcsample_t *buffer, size_t n UNUSED){
    static uint8_t done_samples = 0; // bitfield of completed samples
    int i = 0;
    if(adcp == &ADCD1){
        i = activeport*2;
    }else if(adcp == &ADCD2){
        i = activeport*2 + 1;
    }

    if(adcp == &ADCD1 || adcp == &ADCD2){
        outBuffer[i] = buffer[0];
        done_samples |= 1 << i;
    }

    if(done_samples == 0xFF){
        done_samples = 0;
        chSysLockFromIsr();
        chEvtBroadcastI(&rnhPortCurrent);
        chSysUnlockFromIsr();
    }

}

static void StartADCSample(GPTDriver *gptp UNUSED){
    static adcsample_t buffer0 = 0;
    static adcsample_t buffer1 = 0;
    static ADCConversionGroup bank0 = {
            .circular = FALSE,
            .num_channels = 1,
            .end_cb = ADCCallback,
            .error_cb = NULL,
            .cr1 = ADC_CR1_EOCIE,
            .cr2 = ADC_CR2_SWSTART,
            .smpr1 = ADC_SMPR1_SMP_AN10(ADC_SAMPLE_480),
            .smpr2 = 0,
            .sqr1 = ADC_SQR1_NUM_CH(1),
            .sqr2 = 0,
            .sqr3 = ADC_SQR3_SQ1_N(ADC_CHANNEL_IN10)
    };
    static ADCConversionGroup bank1 = {
            .circular = FALSE,
            .num_channels = 1,
            .end_cb = ADCCallback,
            .error_cb = NULL,
            .cr1 = ADC_CR1_EOCIE,
            .cr2 = ADC_CR2_SWSTART,
            .smpr1 = ADC_SMPR1_SMP_AN11(ADC_SAMPLE_480),
            .smpr2 = 0,
            .sqr1 = ADC_SQR1_NUM_CH(1),
            .sqr2 = 0,
            .sqr3 = ADC_SQR3_SQ1_N(ADC_CHANNEL_IN11)
    };

    select_port_imon(activeport);
    chSysLockFromIsr();
    adcStartConversionI(&ADCD1, &bank0, &buffer0, 1);
    adcStartConversionI(&ADCD2, &bank1, &buffer1, 1);
    chSysUnlockFromIsr();
    activeport = (activeport + 1) % 4;
}


static rnhPortFaultHandler fault_handler = NULL;
static void * fault_handler_data = NULL;

static void portErrorCallback (EXTDriver *extp UNUSED, expchannel_t channel){
    if(fault_handler){
        fault_handler(channel - NUM_PORT, fault_handler_data);
    }
}

void rnhPortStart(void){
    static ADCConfig conf = {0}; //nothing to config on STM32

    adcStart(&ADCD1, &conf);
    adcStart(&ADCD2, &conf);

    static GPTConfig gptcfg = {
            .frequency = 40000,
            .callback = StartADCSample,
            .dier = 0,
    };
    gptStart(&GPTD2, &gptcfg);
    gptStartContinuous(&GPTD2, 1000);


    for(int i = 0; i < NUM_PORT; ++i){
        extAddCallback( &(struct pin){.port=GPIOE, .pad=fault[i]}
                      , EXT_CH_MODE_RISING_EDGE | EXT_CH_MODE_AUTOSTART
                      , portErrorCallback
                      );
    }
    extUtilsStart();
}

RNHPort rnhPortStatus(RNHPort port){
    RNHPort return_port = 0;
    port &= RNH_PORT_ALL;

    for(int i = 0; i < NUM_PORT; ++i){
        if(port & 1<<i){
            return_port |= palReadPad(GPIOE, power[i])<<i;
        }
    }
    return return_port;
};

RNHPort rnhPortFault(RNHPort port){
    RNHPort return_port = 0;
    port &= RNH_PORT_ALL;

    for(int i = 0; i < NUM_PORT; ++i){
        if(port & 1<<i){
            return_port |= palReadPad(GPIOE, fault[i])<<i;
        }
    }
    return return_port;
}

void rnhPortOn(RNHPort port){
    port &= RNH_PORT_ALL;

    for(int i = 0; i < NUM_PORT; ++i){
        if(port & 1<<i){
            palClearPad(GPIOE, power[i]);
        }
    }
}

void rnhPortOff(RNHPort port){
    port &= RNH_PORT_ALL;

    for(int i = 0; i < NUM_PORT; ++i){
        if(port & 1<<i){
            palSetPad(GPIOE, power[i]);
        }
    }
}

void rnhPortSetFaultHandler(rnhPortFaultHandler handler, void * data){
    chSysLock();
    fault_handler = handler;
    fault_handler_data = data;
    chSysUnlock();
}

void rnhPortGetCurrentData(uint16_t * data){
     chSysLock();
     memcpy(data, &outBuffer, sizeof(outBuffer));
     chSysUnlock();
 }

void rnhPortSetCurrentDataRate(unsigned freq){
    gptStopTimer(&GPTD2);
    gptStartContinuous(&GPTD2, 40000 / 4 / freq);
}

static void cmd_port(struct RCICmdData * rci_data, void * user_data UNUSED){
    if(rci_data->cmd_len < 2){
        return; //fixme return error
    }

    RNHAction action = rci_data->cmd_data[0];
    int data = rci_data->cmd_data[1];
    RNHPort ret = 0;

    switch(action){
    case RNH_PORT_STATUS:
        ret = rnhPortStatus(data);
        break;
    case RNH_PORT_FAULT:
        ret = rnhPortFault(data);
        break;
    case RNH_PORT_ON:
        rnhPortOn(data);
        ret = rnhPortStatus(RNH_PORT_ALL);
        break;
    case RNH_PORT_OFF:
        rnhPortOff(data);
        ret = rnhPortStatus(RNH_PORT_ALL);
        break;
    case RNH_PORT_CURRENT_FREQ:
        rnhPortSetCurrentDataRate(data);
        return;
    default:
        return; //fixme return error
    }

    rci_data->return_data[0] = ret;
    rci_data->return_len = 1;
}
