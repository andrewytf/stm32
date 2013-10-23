/*! \file main.c
 *
 * Development for MPU9150 on ChibiOS
 *
 * MPU is an i2c device
 *
 * Includes ADIS SPI connections and development
 *
 * This implementation is specific to the Olimex stm32-e407 board.
 */


/*!
 * \defgroup mainapp Application
 * @{
 */

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "ch.h"
#include "hal.h"

#include "usb_cdc.h"
#include "chprintf.h"
#include "shell.h"

#include "iwdg_lld.h"
#include "usbdetail.h"
#include "extdetail.h"
#include "cmddetail.h"

#include "MPL3115A2.h"

#include "main.h"
#include "board.h"

//#include "lwipopts.h"
//#include "lwipthread.h"

//BaseSequentialStream *chp =  (BaseSequentialStream *)&SDU1;

static Mutex mtx;
static const ShellCommand commands[] = {
		{"mem", cmd_mem},
		{"threads", cmd_threads},
		{NULL, NULL}
};

static const ShellConfig shell_cfg1 = {
		(BaseSequentialStream *)&SDU1,
		commands
};



/*! configure the i2c module on stm32
 *
 */
const I2CConfig mpl3115a2_config = {
    OPMODE_I2C,
    400000,                // i2c clock speed. Test at 400000 when r=4.7k
    FAST_DUTY_CYCLE_2,
    //STD_DUTY_CYCLE,
};

/*! \typedef mpl3115a2_config
 *
 * Configuration for the MPU IMU connections
 */
const mpl3115a2_connect mpl3115a2_connections = {
	GPIOF,                // i2c sda port
	0,                    // i2c_sda_pad
	GPIOF,                // i2c_scl_port
	1,                    // i2c scl pad
	GPIOF,                // interrupt port
	2,                    // interrupt pad;
};

static WORKING_AREA(waThread_blinker, 64);
/*! \brief Green LED blinker thread
 */
static msg_t Thread_blinker(void *arg) {
	(void)arg;
	chRegSetThreadName("blinker");
	while (TRUE) {
		palTogglePad(GPIOC, GPIOC_LED);
		chThdSleepMilliseconds(500);
	}
	return -1;
}
/*
static WORKING_AREA(waThread_mpl3115a2_int, 128);
static msg_t Thread_mpl3115a2_int(void* arg) {
	(void) arg;
	static const evhandler_t evhndl_mpl3115a2[]       = {
			mpl3115a2_int_event_handler
	};
	struct EventListener     evl_mpl3115a2;

	chRegSetThreadName("mpl3115a2_int");

	chEvtRegister(&mpl3115a2_int_event,           &evl_mpl3115a2,         0);

	while (TRUE) {
		chEvtDispatch(evhndl_mpl3115a2, chEvtWaitOneTimeout((EVENT_MASK(2)|EVENT_MASK(1)|EVENT_MASK(0)), MS2ST(50)));
	}
	return -1;
}
*/
static WORKING_AREA(waThread_mpl3115a2, 256);
/*! \brief MPU9150 thread
 */
static msg_t Thread_mpl3115a2(void *arg) {
	(void)arg;
	BaseSequentialStream *chp =  (BaseSequentialStream *)&SDU1;
	//float altitude = 5.0;
	chRegSetThreadName("mpl3115a2");
	
	while (TRUE) {
		//chEvtDispatch(evhndl_newdata, chEvtWaitOneTimeout((eventmask_t)1, US2ST(50)));
		//chprintf(chp,"This is the pressure sensor thread talking\n");
		//altitude = mpl3115a2_get_altitude(mpl3115a2_driver.i2c_instance);
		chMtxLock(&mtx);
		mpl3115a2_get_temperature(mpl3115a2_driver.i2c_instance);
		//chprintf(chp,"High order temperature bits: %d.  Low order temperature bits %d\r\n", mpl3115a2_driver.ho_temp, mpl3115a2_driver.lo_temp);
		chMtxUnlock();
		chprintf(chp,"High order temperature bits: %d.  Low order temperature bits %d\r\n", mpl3115a2_driver.ho_temp, mpl3115a2_driver.lo_temp);
		chMtxLock(&mtx);
		mpl3115a2_get_bar(mpl3115a2_driver.i2c_instance);
		chMtxUnlock();
		chprintf(chp, "High order pressure bits: %d.  Center pressure bits: %d.  Low order pressure bits: %d\r\n",  mpl3115a2_driver.bar_msb,  mpl3115a2_driver.bar_csb,  mpl3115a2_driver.bar_lsb);
		chThdSleepMilliseconds(1000);
	}
	return -1;
}

static WORKING_AREA(waThread_indwatchdog, 64);
/*! \brief  Watchdog thread
 */
static msg_t Thread_indwatchdog(void *arg) {
	(void)arg;

	chRegSetThreadName("iwatchdog");
	while (TRUE) {
		iwdg_lld_reload();
		chThdSleepMilliseconds(250);
	}
	return -1;
}



int main(void) {
	static Thread            *shelltp       = NULL;
	static const evhandler_t evhndl_main[]       = {
			extdetail_WKUP_button_handler
	};
	struct EventListener     el0;

	/*
	 * System initializations.
	 * - HAL initialization, this also initializes the configured device drivers
	 *   and performs the board-specific initializations.
	 * - Kernel initialization, the main() function becomes a thread and the
	 *   RTOS is active.
	 */
	 chMtxInit(&mtx);
	halInit();
	chSysInit();

	extdetail_init();

	/*
	 * I2C2 I/O pins setup.
	 */
	palSetPadMode(mpl3115a2_connections.i2c_sda_port , mpl3115a2_connections.i2c_sda_pad,
			PAL_MODE_ALTERNATE(4) | PAL_STM32_OTYPE_OPENDRAIN | PAL_STM32_OSPEED_HIGHEST |PAL_STM32_PUDR_FLOATING );
	palSetPadMode(mpl3115a2_connections.i2c_scl_port, mpl3115a2_connections.i2c_scl_pad,
			PAL_MODE_ALTERNATE(4) | PAL_STM32_OSPEED_HIGHEST  | PAL_STM32_PUDR_FLOATING);

	/*
	 * MPU9150 Interrupt pin setup
	 */
	palSetPadMode(mpl3115a2_connections.int_port, mpl3115a2_connections.int_pad,
			PAL_MODE_ALTERNATE(4) | PAL_STM32_OSPEED_HIGHEST| PAL_STM32_PUDR_PULLUP | PAL_STM32_MODE_INPUT);

	palSetPad(mpl3115a2_connections.i2c_scl_port,  mpl3115a2_connections.i2c_scl_pad );

	/*!
	 * Initializes a serial-over-USB CDC driver.
	 */
	sduObjectInit(&SDU1);
	sduStart(&SDU1, &serusbcfg);

	/*!
	 * Activates the USB driver and then the USB bus pull-up on D+.
	 * Note, a delay is inserted in order to not have to disconnect the cable
	 * after a reset.
	 */
	usbDisconnectBus(serusbcfg.usbp);
	chThdSleepMilliseconds(1000);
	usbStart(serusbcfg.usbp, &usbcfg);
	usbConnectBus(serusbcfg.usbp);

	shellInit();

	iwdg_begin();

	/*!
	 * Activates the serial driver 6 and SDC driver 1 using default
	 * configuration.
	 */
	//sdStart(&SD6, NULL);

	//spiStart(&SPID1, &adis_spicfg);       /* Set transfer parameters.  */

	mpl3115a2_start(&I2CD2);
	i2cStart(mpl3115a2_driver.i2c_instance, &mpl3115a2_config);
	chThdSleepMilliseconds(1000);
	mpl3115a2_init(&I2CD2);
	/*! Activates the EXT driver 1. */
	//extStart(&EXTD1, &extcfg);

	chThdCreateStatic(waThread_blinker,      sizeof(waThread_blinker),      NORMALPRIO, Thread_blinker,      NULL);
	chThdCreateStatic(waThread_indwatchdog,  sizeof(waThread_indwatchdog),  NORMALPRIO, Thread_indwatchdog,  NULL);
	chThdCreateStatic(waThread_mpl3115a2,      sizeof(waThread_mpl3115a2),      NORMALPRIO, Thread_mpl3115a2,      NULL);

	chEvtRegister(&extdetail_wkup_event, &el0, 0);
	while (TRUE) {
		if (!shelltp && (SDU1.config->usbp->state == USB_ACTIVE))
			shelltp = shellCreate(&shell_cfg1, SHELL_WA_SIZE, NORMALPRIO);
		else if (chThdTerminated(shelltp)) {
			chThdRelease(shelltp);    /* Recovers memory of the previous shell.   */
			shelltp = NULL;           /* Triggers spawning of a new shell.        */
		}
		chEvtDispatch(evhndl_main, chEvtWaitOneTimeout((eventmask_t)1, MS2ST(500)));
	}
}


//! @}