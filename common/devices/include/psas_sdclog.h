/*! \file psas_sdclog.h
 *
 */

#ifndef PSAS_SDCLOG_H_
#define PSAS_SDCLOG_H_

/*!
 * \addtogroup psas_sdclog
 * @{
 */
#include "ch.h"
#include "hal.h"
#include "ff.h"

#include "psas_rtc.h"
#include "usbdetail.h"

#include "crc_16_reflect.h"

#ifdef __cplusplus
extern "C" {
#endif

/* A 162 byte message written at 1000hz will use 4GB in about 6.5 Hours */
#define         SDC_MAX_PAYLOAD_BYTES                       150
#define         SDC_NUM_ID_CHARS                            4


    extern          bool            fs_ready;
    extern          FATFS           SDC_FS;
    extern          DWORD           sdc_fp_index;
    extern          DWORD           sdc_fp_index_old;

    extern          EventSource     sdc_inserted_event;
    extern          EventSource     sdc_removed_event;


    // create end of data fiducials...slightly larger than log entry
    typedef struct sdc_eod_marker {
        // GENERIC_message + checksum + eod_marker + eod_marker
        uint8_t sdc_eodmarks[166+2+2+2];
    } sdc_eod_marker;

    typedef enum SDC_ERRORCode {
        SDC_OK                   = 0,
        SDC_NULL_PARAMETER_ERROR = -1,
        SDC_FSYNC_ERROR          = -2,
        SDC_FWRITE_ERROR         = -3,
        SDC_ASSERT_ERROR         = -4,
        SDC_UNKNOWN_ERROR        = -99
    } SDC_ERRORCode;

    struct Message_head {
        char                 ID[SDC_NUM_ID_CHARS];         // This must be first part of data. Reserved value: 0xa5a5
        uint32_t             index;
        psas_timespec        ts;
        uint16_t             data_length;
    } __attribute__((packed));
    typedef struct Message_head Message_head;

    /*! RTCLogtime is not psas_timespec. 
     *      psas_timespec is time since FC reboot 
     *      RTCLogtime    is the RTC on the STM32, running on a backup battery.
     */
    struct RTCLogtime {
      time_t   tv_time;
      uint32_t tv_msec;
    } __attribute__((packed));
    typedef struct RTCLogtime RTCLogtime;

    struct GENERIC_message {
        Message_head         mh;                           // 16 bytes
        RTCLogtime           logtime;                      // 12 bytes
        uint8_t              data[SDC_MAX_PAYLOAD_BYTES];  // 150 bytes
    } __attribute__((packed));
    typedef struct GENERIC_message GENERIC_message;

    static inline void sdc_reset_fp_index(void) {
        sdc_fp_index = 0;
    }

    void            sdc_insert_handler(eventid_t id) ;
    void            sdc_remove_handler(eventid_t id) ;
    void            sdc_tmr_init(void *p) ;
    void            sdc_set_fp_index(FIL* DATAFil, DWORD ofs) ;
    void            sdc_init_eod (uint8_t marker_byte) ;

    FRESULT         sdc_write_checksum(FIL* DATAFil, const crc_t* d, uint32_t* bw) ;
    FRESULT         sdc_write_log_message(FIL* DATAFil, GENERIC_message* d, uint32_t * bw) ;
    FRESULT         sdc_scan_files(BaseSequentialStream *chp, char *path) ;

    /*! \todo implement sdc_seek_eod function. */
    void            sdc_seek_eod(FIL* DATAFil, GENERIC_message* d, uint32_t* sdindexbyte) ;

#ifdef __cplusplus
}
#endif
//! @}

#endif
