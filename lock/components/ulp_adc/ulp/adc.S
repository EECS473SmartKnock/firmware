/* 
    Credits for backbone of this code: 
    https://github.com/SensorsIot/ESP32-ULP-Arduino-IDE/blob/master/ulp_adc/adc.s

    Must allocate more memory for the ulp in
    Arduino15\packages\esp32\hardware\esp32\1.0.0\tools\sdk\sdkconfig.h
    -> #define CONFIG_ULP_COPROC_RESERVE_MEM
    for this sketch to compile. 2048b seems good

    ULP assembly files are passed through C preprocessor first, so include directives
    and C macros may be used in these files
*/
#include "soc/rtc_cntl_reg.h"
#include "soc/rtc_io_reg.h"
#include "soc/soc_ulp.h"

/* ADC1 channel 6, GPIO34 */
.set adc_channel, 6

/* Configure the number of ADC samples to average on each measurement.
   For convenience, make it a power of 2. */
.set adc_oversampling_factor_log, 1
.set adc_oversampling_factor, (1 << adc_oversampling_factor_log)

/* Define variables, which go into .bss section (zero-initialized data) */
.bss

/* Low threshold of ADC reading.
   Set by the main program. */
.global low_threshold
low_threshold: .long 0

/* High threshold of ADC reading.
   Set by the main program. */
.global high_threshold
high_threshold: .long 0

/* Counter of measurements done */
.global sample_counter
sample_counter:
.long 0

.global ADC_reading
ADC_reading:
.long 0

/* Code goes into .text section */
.text
.global entry
entry:
/* Disable hold of GPIO13 output */
WRITE_RTC_REG(RTC_IO_TOUCH_PAD4_REG, RTC_IO_TOUCH_PAD4_HOLD_S, 1, 0)
/* Set the GPIO13 output HIGH */
WRITE_RTC_REG(RTC_GPIO_OUT_W1TS_REG, RTC_GPIO_OUT_DATA_W1TS_S + 14, 1, 1)

/* increment sample counter */
move r3, sample_counter
ld R2, r3, 0
add R2, R2, 1
st R2, r3, 0

/* do measurements using ADC */
/* r0 will be used as accumulator */
move r0, 0
/* initialize the loop counter */
stage_rst

measure:
/* measure and add value to accumulator */
adc r1, 0, adc_channel + 1
add r0, r0, r1
/* increment loop counter and check exit condition */
stage_inc 1
jumps measure, adc_oversampling_factor, lt

/* divide accumulator by adc_oversampling_factor.
   Since it is chosen as a power of two, use right shift */
rsh r0, r0, adc_oversampling_factor_log
/* averaged value is now in r0; store it into ADC_reading */
move r3, ADC_reading
st r0, r3, 0


/* compare with low_threshold; wake up if value < low_threshold */
; move r3, low_threshold
; ld r3, r3, 0
; sub r3, r0, r3
; jump led_off, ov


/* compare with high_threshold; wake up if value > high_threshold */
move r3, high_threshold
ld r3, r3, 0
sub r3, r3, r0
jump wake_up, ov

led_on:
//** LED on
/* Disable hold on GPIO27 output */
WRITE_RTC_REG(RTC_IO_TOUCH_PAD7_REG, RTC_IO_TOUCH_PAD7_HOLD_S, 1, 0)
/* Set the GPIO27 output HIGH (clear output) to signal that ULP is now going down */
WRITE_RTC_REG(RTC_GPIO_OUT_W1TS_REG, RTC_GPIO_OUT_DATA_W1TS_S + 17, 1, 1)
/* Enable hold on GPIO27 output */
WRITE_RTC_REG(RTC_IO_TOUCH_PAD7_REG, RTC_IO_TOUCH_PAD7_HOLD_S, 1, 1)
jump exit


led_off:
 // ** LED Off
/* Disable hold on GPIO27 output */
WRITE_RTC_REG(RTC_IO_TOUCH_PAD7_REG, RTC_IO_TOUCH_PAD7_HOLD_S, 1, 0)
/* Set the GPIO27 output LOW (clear output) to signal that ULP is now going down */
WRITE_RTC_REG(RTC_GPIO_OUT_W1TC_REG, RTC_GPIO_OUT_DATA_W1TC_S + 17, 1, 1)
/* Enable hold on GPIO27 output */
WRITE_RTC_REG(RTC_IO_TOUCH_PAD7_REG, RTC_IO_TOUCH_PAD7_HOLD_S, 1, 1)
jump exit


/* value within range, end the program */
.global exit
exit:
/* Set the GPIO13 output LOW (clear output) to signal that ULP is now going down */
WRITE_RTC_REG(RTC_GPIO_OUT_W1TC_REG, RTC_GPIO_OUT_DATA_W1TC_S + 14, 1, 1)
/* Enable hold on GPIO13 output */
WRITE_RTC_REG(RTC_IO_TOUCH_PAD4_REG, RTC_IO_TOUCH_PAD4_HOLD_S, 1, 1)
halt

.global wake_up
wake_up:
/* Check if the system can be woken up */
READ_RTC_FIELD(RTC_CNTL_LOW_POWER_ST_REG, RTC_CNTL_RDY_FOR_WAKEUP)
and r0, r0, 1
jump exit, eq

/* Wake up the SoC, end program */
wake
WRITE_RTC_FIELD(RTC_CNTL_STATE0_REG, RTC_CNTL_ULP_CP_SLP_TIMER_EN, 0)
jump exit
