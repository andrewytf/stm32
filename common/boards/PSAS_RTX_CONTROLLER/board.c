/*
    ChibiOS/RT - Copyright (C) 2006,2007,2008,2009,2010,
                 2011,2012 Giovanni Di Sirio.

    This file is part of ChibiOS/RT.

    ChibiOS/RT is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    ChibiOS/RT is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "ch.h"
#include "hal.h"
#include "mii.h"
#if HAL_USE_PAL || defined(__DOXYGEN__)
/**
 * @brief   PAL setup.
 * @details Digital I/O ports static configuration as defined in @p board.h.
 *          This variable is used by the HAL when initializing the PAL driver.
 */
const PALConfig pal_default_config =
{
  {VAL_GPIOA_MODER, VAL_GPIOA_OTYPER, VAL_GPIOA_OSPEEDR, VAL_GPIOA_PUPDR,
   VAL_GPIOA_ODR,   VAL_GPIOA_AFRL,   VAL_GPIOA_AFRH},
  {VAL_GPIOB_MODER, VAL_GPIOB_OTYPER, VAL_GPIOB_OSPEEDR, VAL_GPIOB_PUPDR,
   VAL_GPIOB_ODR,   VAL_GPIOB_AFRL,   VAL_GPIOB_AFRH},
  {VAL_GPIOC_MODER, VAL_GPIOC_OTYPER, VAL_GPIOC_OSPEEDR, VAL_GPIOC_PUPDR,
   VAL_GPIOC_ODR,   VAL_GPIOC_AFRL,   VAL_GPIOC_AFRH},
  {VAL_GPIOD_MODER, VAL_GPIOD_OTYPER, VAL_GPIOD_OSPEEDR, VAL_GPIOD_PUPDR,
   VAL_GPIOD_ODR,   VAL_GPIOD_AFRL,   VAL_GPIOD_AFRH},
  {VAL_GPIOE_MODER, VAL_GPIOE_OTYPER, VAL_GPIOE_OSPEEDR, VAL_GPIOE_PUPDR,
   VAL_GPIOE_ODR,   VAL_GPIOE_AFRL,   VAL_GPIOE_AFRH},
  {0, 0, 0, 0,
   0,   0,   0},
  {0, 0, 0, 0,
   0,   0,   0},
  {VAL_GPIOH_MODER, VAL_GPIOH_OTYPER, VAL_GPIOH_OSPEEDR, VAL_GPIOH_PUPDR,
   VAL_GPIOH_ODR,   VAL_GPIOH_AFRL,   0},
  {0, 0, 0, 0,
   0,   0,   0}
};
#endif

/**
 * @brief   Early initialization code.
 * @details This initialization must be performed just after stack setup
 *          and before any other initialization.
 */
void __early_init(void) {

  stm32_clock_init();
}

#if HAL_USE_MAC || defined(__DOXYGEN__)
/**
 * @breif Ethernet PHY reset
 */

#define PHYCTRL2 0x1F
#define RMII_RCLKSEL (1<<7)
#define BOARD_PHY_HARD_RESET_DELAY US2RTT(100)
#define BOARD_PHY_RESET_DELAY
/* mii_read, mii_write and MACMIIDR_CR copyied over from mac_lld */

/* MII divider optimal value.*/
#if (STM32_HCLK >= 150000000)
#define MACMIIDR_CR ETH_MACMIIAR_CR_Div102
#elif (STM32_HCLK >= 100000000)
#define MACMIIDR_CR ETH_MACMIIAR_CR_Div62
#elif (STM32_HCLK >= 60000000)
#define MACMIIDR_CR     ETH_MACMIIAR_CR_Div42
#elif (STM32_HCLK >= 35000000)
#define MACMIIDR_CR     ETH_MACMIIAR_CR_Div26
#elif (STM32_HCLK >= 20000000)
#define MACMIIDR_CR     ETH_MACMIIAR_CR_Div16
#else
#error "STM32_HCLK below minimum frequency for ETH operations (20MHz)"
#endif

/**
 * @brief   Writes a PHY register.
 *
 * @param[in] macp      pointer to the @p MACDriver object
 * @param[in] reg       register number
 * @param[in] value     new register value
 */
static void mii_write(MACDriver *macp, uint32_t reg, uint32_t value) {

  ETH->MACMIIDR = value;
  ETH->MACMIIAR = macp->phyaddr | (reg << 6) | MACMIIDR_CR |
                  ETH_MACMIIAR_MW | ETH_MACMIIAR_MB;
  while ((ETH->MACMIIAR & ETH_MACMIIAR_MB) != 0)
    ;
}

/**
 * @brief   Reads a PHY register.
 *
 * @param[in] macp      pointer to the @p MACDriver object
 * @param[in] reg       register number
 *
 * @return              The PHY register content.
 */
static uint32_t mii_read(MACDriver *macp, uint32_t reg) {
  ETH->MACMIIAR = macp->phyaddr | (reg << 6) | MACMIIDR_CR | ETH_MACMIIAR_MB;
  while ((ETH->MACMIIAR & ETH_MACMIIAR_MB) != 0)
    ;
  return ETH->MACMIIDR;
}

void mac_phy_reset(void){
  palSetPad(GPIOD, GPIOE_EPHY_NRST);
  halPolledDelay(BOARD_PHY_HARD_RESET_DELAY);
  /* PHY soft reset procedure.*/
  mii_write(&ETHD1, MII_BMCR, BMCR_RESET);
  while (mii_read(&ETHD1, MII_BMCR) & BMCR_RESET)
    ;
  mii_write(&ETHD1, PHYCTRL2, mii_read(&ETHD1, PHYCTRL2) | RMII_RCLKSEL);
}
#endif /* HAL_USE_MAC */

/**
 * @brief   Board-specific initialization code.
 */
void boardInit(void) {
}
