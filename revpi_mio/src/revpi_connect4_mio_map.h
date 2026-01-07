#ifndef REVPI_CONNECT4_MIO_MAP_H
#define REVPI_CONNECT4_MIO_MAP_H

/*
 * Process image layout for:
 *   - RevPi Connect 4 (BASE, productType 136) at position 0
 *   - RevPi MIO       (LEFT_RIGHT, productType 118) at position 32
 *
 * Source: config_mio_jq.json + piTest -d / -v
 *
 * All OFFSETS below are absolute byte offsets in the global process image
 * as seen by /dev/piControl0 and piTest.
 *
 * Bit positions are 0..7 within the byte at that OFFSET.
 *
 * Connect 4:
 *   Input  region: offset 0,  length 6
 *   Output region: offset 6,  length 7
 *
 * MIO:
 *   Input  region: offset 13, length 34
 *   Output region: offset 47, length 27
 *   Memory region: based at MIO_INPUT_BASE (13) + mem.ByteOff
 */

/* ------------------------------------------------------------------------- */
/* Global base offsets (from piTest -d)                                      */
/* ------------------------------------------------------------------------- */

#define C4_INPUT_BASE          0   /* RevPi Connect 4 input base */
#define C4_OUTPUT_BASE         6   /* RevPi Connect 4 output base */

#define MIO_INPUT_BASE        13   /* RevPi MIO input base */
#define MIO_OUTPUT_BASE       47   /* RevPi MIO output base */

/*
 * Memory (mem) for MIO uses offsets relative to MIO_INPUT_BASE.
 * Absolute mem offset = MIO_INPUT_BASE + mem.ByteOff
 */

/* ------------------------------------------------------------------------- */
/* RevPi Connect 4 – Inputs (inp)                                            */
/* ------------------------------------------------------------------------- */

/* RevPiStatus (8 bit, ByteOff=0) */
#define C4_IN_REVPISTATUS_OFFSET     (C4_INPUT_BASE + 0)   /* = 0 */
#define C4_IN_REVPISTATUS_LEN_BITS   8

/* RevPiIOCycle (8 bit, ByteOff=1) */
#define C4_IN_REVPICYCLE_OFFSET      (C4_INPUT_BASE + 1)   /* = 1 */
#define C4_IN_REVPICYCLE_LEN_BITS    8

/* RS485ErrorCnt (16 bit, ByteOff=2) */
#define C4_IN_RS485_ERRCNT_OFFSET    (C4_INPUT_BASE + 2)   /* = 2 */
#define C4_IN_RS485_ERRCNT_LEN_BITS 16

/* Core_Temperature (8 bit, ByteOff=4) */
#define C4_IN_CORE_TEMP_OFFSET       (C4_INPUT_BASE + 4)   /* = 4 */
#define C4_IN_CORE_TEMP_LEN_BITS     8

/* Core_Frequency (8 bit, ByteOff=5) */
#define C4_IN_CORE_FREQ_OFFSET       (C4_INPUT_BASE + 5)   /* = 5 */
#define C4_IN_CORE_FREQ_LEN_BITS     8

/* ------------------------------------------------------------------------- */
/* RevPi Connect 4 – Outputs (out)                                           */
/* ------------------------------------------------------------------------- */

/* RevPiOutput (8 bit, ByteOff=6) */
#define C4_OUT_REVPIOUT_OFFSET       (C4_OUTPUT_BASE + 0)  /* = 6 */
#define C4_OUT_REVPIOUT_LEN_BITS     8

/* RS485ErrorLimit1 (16 bit, ByteOff=7) */
#define C4_OUT_RS485_ERRLIM1_OFFSET  (C4_OUTPUT_BASE + 1)  /* = 7 */
#define C4_OUT_RS485_ERRLIM1_LEN_BITS 16

/* RS485ErrorLimit2 (16 bit, ByteOff=9) */
#define C4_OUT_RS485_ERRLIM2_OFFSET  (C4_OUTPUT_BASE + 3)  /* = 9 */
#define C4_OUT_RS485_ERRLIM2_LEN_BITS 16

/* RevPiLED (16 bit, ByteOff=11) */
#define C4_OUT_REVPILED_OFFSET       (C4_OUTPUT_BASE + 5)  /* = 11 */
#define C4_OUT_REVPILED_LEN_BITS     16

/* ------------------------------------------------------------------------- */
/* RevPi MIO – Inputs (inp)                                                  */
/* ------------------------------------------------------------------------- */

/*
 * NOTE: for MIO inputs, absolute offset = MIO_INPUT_BASE + inp.ByteOff
 */

/* DigitalInput_1..4 (1 bit each, ByteOff=0, bits 0..3) */
#define MIO_IN_DI_BYTE_OFFSET        (MIO_INPUT_BASE + 0)  /* = 13 */

#define MIO_IN_DI1_OFFSET            MIO_IN_DI_BYTE_OFFSET
#define MIO_IN_DI1_BIT               0
#define MIO_IN_DI1_LEN_BITS          1

#define MIO_IN_DI2_OFFSET            MIO_IN_DI_BYTE_OFFSET
#define MIO_IN_DI2_BIT               1
#define MIO_IN_DI2_LEN_BITS          1

#define MIO_IN_DI3_OFFSET            MIO_IN_DI_BYTE_OFFSET
#define MIO_IN_DI3_BIT               2
#define MIO_IN_DI3_LEN_BITS          1

#define MIO_IN_DI4_OFFSET            MIO_IN_DI_BYTE_OFFSET
#define MIO_IN_DI4_BIT               3
#define MIO_IN_DI4_LEN_BITS          1

/* ReservedDI_5..8 (1 bit each, ByteOff=0, bits 4..7) */
#define MIO_IN_DI5_OFFSET            MIO_IN_DI_BYTE_OFFSET
#define MIO_IN_DI5_BIT               4
#define MIO_IN_DI5_LEN_BITS          1

#define MIO_IN_DI6_OFFSET            MIO_IN_DI_BYTE_OFFSET
#define MIO_IN_DI6_BIT               5
#define MIO_IN_DI6_LEN_BITS          1

#define MIO_IN_DI7_OFFSET            MIO_IN_DI_BYTE_OFFSET
#define MIO_IN_DI7_BIT               6
#define MIO_IN_DI7_LEN_BITS          1

#define MIO_IN_DI8_OFFSET            MIO_IN_DI_BYTE_OFFSET
#define MIO_IN_DI8_BIT               7
#define MIO_IN_DI8_LEN_BITS          1

/* DutyCycle_PulseLength_1..4 (16 bit, ByteOff=1,3,5,7) */
#define MIO_IN_DUTY_PULSE1_OFFSET    (MIO_INPUT_BASE + 1)
#define MIO_IN_DUTY_PULSE1_LEN_BITS  16

#define MIO_IN_DUTY_PULSE2_OFFSET    (MIO_INPUT_BASE + 3)
#define MIO_IN_DUTY_PULSE2_LEN_BITS  16

#define MIO_IN_DUTY_PULSE3_OFFSET    (MIO_INPUT_BASE + 5)
#define MIO_IN_DUTY_PULSE3_LEN_BITS  16

#define MIO_IN_DUTY_PULSE4_OFFSET    (MIO_INPUT_BASE + 7)
#define MIO_IN_DUTY_PULSE4_LEN_BITS  16

/* Fpwm_PulseCount_1..4 (16 bit, ByteOff=9,11,13,15) */
#define MIO_IN_FPWM_PULSECOUNT1_OFFSET  (MIO_INPUT_BASE + 9)
#define MIO_IN_FPWM_PULSECOUNT1_LEN_BITS 16

#define MIO_IN_FPWM_PULSECOUNT2_OFFSET  (MIO_INPUT_BASE + 11)
#define MIO_IN_FPWM_PULSECOUNT2_LEN_BITS 16

#define MIO_IN_FPWM_PULSECOUNT3_OFFSET  (MIO_INPUT_BASE + 13)
#define MIO_IN_FPWM_PULSECOUNT3_LEN_BITS 16

#define MIO_IN_FPWM_PULSECOUNT4_OFFSET  (MIO_INPUT_BASE + 15)
#define MIO_IN_FPWM_PULSECOUNT4_LEN_BITS 16

/* AnalogInputLogicLevel_1..8 (1 bit each, ByteOff=17, bits 0..7) */
#define MIO_IN_AI_LOGIC_BYTE_OFFSET  (MIO_INPUT_BASE + 17)

#define MIO_IN_AI1_LOGIC_OFFSET      MIO_IN_AI_LOGIC_BYTE_OFFSET
#define MIO_IN_AI1_LOGIC_BIT         0
#define MIO_IN_AI1_LOGIC_LEN_BITS    1

#define MIO_IN_AI2_LOGIC_OFFSET      MIO_IN_AI_LOGIC_BYTE_OFFSET
#define MIO_IN_AI2_LOGIC_BIT         1
#define MIO_IN_AI2_LOGIC_LEN_BITS    1

#define MIO_IN_AI3_LOGIC_OFFSET      MIO_IN_AI_LOGIC_BYTE_OFFSET
#define MIO_IN_AI3_LOGIC_BIT         2
#define MIO_IN_AI3_LOGIC_LEN_BITS    1

#define MIO_IN_AI4_LOGIC_OFFSET      MIO_IN_AI_LOGIC_BYTE_OFFSET
#define MIO_IN_AI4_LOGIC_BIT         3
#define MIO_IN_AI4_LOGIC_LEN_BITS    1

#define MIO_IN_AI5_LOGIC_OFFSET      MIO_IN_AI_LOGIC_BYTE_OFFSET
#define MIO_IN_AI5_LOGIC_BIT         4
#define MIO_IN_AI5_LOGIC_LEN_BITS    1

#define MIO_IN_AI6_LOGIC_OFFSET      MIO_IN_AI_LOGIC_BYTE_OFFSET
#define MIO_IN_AI6_LOGIC_BIT         5
#define MIO_IN_AI6_LOGIC_LEN_BITS    1

#define MIO_IN_AI7_LOGIC_OFFSET      MIO_IN_AI_LOGIC_BYTE_OFFSET
#define MIO_IN_AI7_LOGIC_BIT         6
#define MIO_IN_AI7_LOGIC_LEN_BITS    1

#define MIO_IN_AI8_LOGIC_OFFSET      MIO_IN_AI_LOGIC_BYTE_OFFSET
#define MIO_IN_AI8_LOGIC_BIT         7
#define MIO_IN_AI8_LOGIC_LEN_BITS    1

/* AnalogInput_1..8 (16 bit, ByteOff=18,20,22,24,26,28,30,32) */
#define MIO_IN_AI1_OFFSET            (MIO_INPUT_BASE + 18)
#define MIO_IN_AI1_LEN_BITS          16

#define MIO_IN_AI2_OFFSET            (MIO_INPUT_BASE + 20)
#define MIO_IN_AI2_LEN_BITS          16

#define MIO_IN_AI3_OFFSET            (MIO_INPUT_BASE + 22)
#define MIO_IN_AI3_LEN_BITS          16

#define MIO_IN_AI4_OFFSET            (MIO_INPUT_BASE + 24)
#define MIO_IN_AI4_LEN_BITS          16

#define MIO_IN_AI5_OFFSET            (MIO_INPUT_BASE + 26)
#define MIO_IN_AI5_LEN_BITS          16

#define MIO_IN_AI6_OFFSET            (MIO_INPUT_BASE + 28)
#define MIO_IN_AI6_LEN_BITS          16

#define MIO_IN_AI7_OFFSET            (MIO_INPUT_BASE + 30)
#define MIO_IN_AI7_LEN_BITS          16

#define MIO_IN_AI8_OFFSET            (MIO_INPUT_BASE + 32)
#define MIO_IN_AI8_LEN_BITS          16

/* ------------------------------------------------------------------------- */
/* RevPi MIO – Outputs (out)                                                 */
/* ------------------------------------------------------------------------- */

/*
 * NOTE: for MIO outputs, absolute offset = MIO_OUTPUT_BASE + (out.ByteOff - 34)
 * since DigitalOutput_1..8 all have ByteOff=34 in JSON and start at offset 47.
 */

/* Shared DO byte for DigitalOutput_1..4 and ReservedDO_5..8 */
#define MIO_OUT_DO_BYTE_OFFSET       (MIO_OUTPUT_BASE + 0) /* 47 = 13 + 34 */

/* DigitalOutput_1..4 (1 bit each, bits 0..3) */
#define MIO_OUT_DO1_OFFSET           MIO_OUT_DO_BYTE_OFFSET
#define MIO_OUT_DO1_BIT              0
#define MIO_OUT_DO1_LEN_BITS         1

#define MIO_OUT_DO2_OFFSET           MIO_OUT_DO_BYTE_OFFSET
#define MIO_OUT_DO2_BIT              1
#define MIO_OUT_DO2_LEN_BITS         1

#define MIO_OUT_DO3_OFFSET           MIO_OUT_DO_BYTE_OFFSET
#define MIO_OUT_DO3_BIT              2
#define MIO_OUT_DO3_LEN_BITS         1

#define MIO_OUT_DO4_OFFSET           MIO_OUT_DO_BYTE_OFFSET
#define MIO_OUT_DO4_BIT              3
#define MIO_OUT_DO4_LEN_BITS         1

/* ReservedDO_5..8 (1 bit each, bits 4..7) */
#define MIO_OUT_DO5_OFFSET           MIO_OUT_DO_BYTE_OFFSET
#define MIO_OUT_DO5_BIT              4
#define MIO_OUT_DO5_LEN_BITS         1

#define MIO_OUT_DO6_OFFSET           MIO_OUT_DO_BYTE_OFFSET
#define MIO_OUT_DO6_BIT              5
#define MIO_OUT_DO6_LEN_BITS         1

#define MIO_OUT_DO7_OFFSET           MIO_OUT_DO_BYTE_OFFSET
#define MIO_OUT_DO7_BIT              6
#define MIO_OUT_DO7_LEN_BITS         1

#define MIO_OUT_DO8_OFFSET           MIO_OUT_DO_BYTE_OFFSET
#define MIO_OUT_DO8_BIT              7
#define MIO_OUT_DO8_LEN_BITS         1

/* PwmDutycycle_1..4 (16 bit, ByteOff=35,37,39,41) */
#define MIO_OUT_PWM_DUTY1_OFFSET     (MIO_OUTPUT_BASE + 1) /* 48 */
#define MIO_OUT_PWM_DUTY1_LEN_BITS   16

#define MIO_OUT_PWM_DUTY2_OFFSET     (MIO_OUTPUT_BASE + 3) /* 50 */
#define MIO_OUT_PWM_DUTY2_LEN_BITS   16

#define MIO_OUT_PWM_DUTY3_OFFSET     (MIO_OUTPUT_BASE + 5) /* 52 */
#define MIO_OUT_PWM_DUTY3_LEN_BITS   16

#define MIO_OUT_PWM_DUTY4_OFFSET     (MIO_OUTPUT_BASE + 7) /* 54 */
#define MIO_OUT_PWM_DUTY4_LEN_BITS   16

/* AnalogOutputLogicLevel_1..8 (1 bit each, ByteOff=43, bits 0..7) */
#define MIO_OUT_AO_LOGIC_BYTE_OFFSET (MIO_OUTPUT_BASE + 9) /* 56 */

#define MIO_OUT_AO1_LOGIC_OFFSET     MIO_OUT_AO_LOGIC_BYTE_OFFSET
#define MIO_OUT_AO1_LOGIC_BIT        0
#define MIO_OUT_AO1_LOGIC_LEN_BITS   1

#define MIO_OUT_AO2_LOGIC_OFFSET     MIO_OUT_AO_LOGIC_BYTE_OFFSET
#define MIO_OUT_AO2_LOGIC_BIT        1
#define MIO_OUT_AO2_LOGIC_LEN_BITS   1

#define MIO_OUT_AO3_LOGIC_OFFSET     MIO_OUT_AO_LOGIC_BYTE_OFFSET
#define MIO_OUT_AO3_LOGIC_BIT        2
#define MIO_OUT_AO3_LOGIC_LEN_BITS   1

#define MIO_OUT_AO4_LOGIC_OFFSET     MIO_OUT_AO_LOGIC_BYTE_OFFSET
#define MIO_OUT_AO4_LOGIC_BIT        3
#define MIO_OUT_AO4_LOGIC_LEN_BITS   1

#define MIO_OUT_AO5_LOGIC_OFFSET     MIO_OUT_AO_LOGIC_BYTE_OFFSET
#define MIO_OUT_AO5_LOGIC_BIT        4
#define MIO_OUT_AO5_LOGIC_LEN_BITS   1

#define MIO_OUT_AO6_LOGIC_OFFSET     MIO_OUT_AO_LOGIC_BYTE_OFFSET
#define MIO_OUT_AO6_LOGIC_BIT        5
#define MIO_OUT_AO6_LOGIC_LEN_BITS   1

#define MIO_OUT_AO7_LOGIC_OFFSET     MIO_OUT_AO_LOGIC_BYTE_OFFSET
#define MIO_OUT_AO7_LOGIC_BIT        6
#define MIO_OUT_AO7_LOGIC_LEN_BITS   1

#define MIO_OUT_AO8_LOGIC_OFFSET     MIO_OUT_AO_LOGIC_BYTE_OFFSET
#define MIO_OUT_AO8_LOGIC_BIT        7
#define MIO_OUT_AO8_LOGIC_LEN_BITS   1

/* AnalogOutput_1..8 (16 bit, ByteOff=45,47,49,51,53,55,57,59) */
#define MIO_OUT_AO1_OFFSET           (MIO_OUTPUT_BASE + 11) /* 58 */
#define MIO_OUT_AO1_LEN_BITS         16

#define MIO_OUT_AO2_OFFSET           (MIO_OUTPUT_BASE + 13) /* 60 */
#define MIO_OUT_AO2_LEN_BITS         16

#define MIO_OUT_AO3_OFFSET           (MIO_OUTPUT_BASE + 15) /* 62 */
#define MIO_OUT_AO3_LEN_BITS         16

#define MIO_OUT_AO4_OFFSET           (MIO_OUTPUT_BASE + 17) /* 64 */
#define MIO_OUT_AO4_LEN_BITS         16

#define MIO_OUT_AO5_OFFSET           (MIO_OUTPUT_BASE + 19) /* 66 */
#define MIO_OUT_AO5_LEN_BITS         16

#define MIO_OUT_AO6_OFFSET           (MIO_OUTPUT_BASE + 21) /* 68 */
#define MIO_OUT_AO6_LEN_BITS         16

#define MIO_OUT_AO7_OFFSET           (MIO_OUTPUT_BASE + 23) /* 70 */
#define MIO_OUT_AO7_LEN_BITS         16

#define MIO_OUT_AO8_OFFSET           (MIO_OUTPUT_BASE + 25) /* 72 */
#define MIO_OUT_AO8_LEN_BITS         16

/* Reserved (8 bit, ByteOff=44 -> absolute 57) */
#define MIO_OUT_RESERVED_BYTE_OFFSET (MIO_OUTPUT_BASE + 10) /* 57 */
#define MIO_OUT_RESERVED_LEN_BITS    8

/* ------------------------------------------------------------------------- */
/* RevPi MIO – Memory (mem)                                                  */
/* ------------------------------------------------------------------------- */

/*
 * Absolute mem offset = MIO_INPUT_BASE + mem.ByteOff
 */

/* EncoderMode (8 bit, ByteOff=61 -> 13+61=74) */
#define MIO_MEM_ENCODERMODE_OFFSET      (MIO_INPUT_BASE + 61) /* 74 */
#define MIO_MEM_ENCODERMODE_LEN_BITS    8

/* IO_Mode_1..4 (8 bit, ByteOff=62..65) */
#define MIO_MEM_IO_MODE1_OFFSET         (MIO_INPUT_BASE + 62) /* 75 */
#define MIO_MEM_IO_MODE1_LEN_BITS       8

#define MIO_MEM_IO_MODE2_OFFSET         (MIO_INPUT_BASE + 63) /* 76 */
#define MIO_MEM_IO_MODE2_LEN_BITS       8

#define MIO_MEM_IO_MODE3_OFFSET         (MIO_INPUT_BASE + 64) /* 77 */
#define MIO_MEM_IO_MODE3_LEN_BITS       8

#define MIO_MEM_IO_MODE4_OFFSET         (MIO_INPUT_BASE + 65) /* 78 */
#define MIO_MEM_IO_MODE4_LEN_BITS       8

/* Pullup (8 bit, ByteOff=66) */
#define MIO_MEM_PULLUP_OFFSET           (MIO_INPUT_BASE + 66) /* 79 */
#define MIO_MEM_PULLUP_LEN_BITS         8

/* PulseMode (8 bit, ByteOff=67) */
#define MIO_MEM_PULSEMODE_OFFSET        (MIO_INPUT_BASE + 67) /* 80 */
#define MIO_MEM_PULSEMODE_LEN_BITS      8

/* FpwmOut_12, _3, _4 (16 bit, ByteOff=68,70,72) */
#define MIO_MEM_FPWMOUT_12_OFFSET       (MIO_INPUT_BASE + 68) /* 81 */
#define MIO_MEM_FPWMOUT_12_LEN_BITS     16

#define MIO_MEM_FPWMOUT_3_OFFSET        (MIO_INPUT_BASE + 70) /* 83 */
#define MIO_MEM_FPWMOUT_3_LEN_BITS      16

#define MIO_MEM_FPWMOUT_4_OFFSET        (MIO_INPUT_BASE + 72) /* 85 */
#define MIO_MEM_FPWMOUT_4_LEN_BITS      16

/* PulseLength_1..4 (16 bit, ByteOff=74,76,78,80) */
#define MIO_MEM_PULSELEN1_OFFSET        (MIO_INPUT_BASE + 74) /* 87 */
#define MIO_MEM_PULSELEN1_LEN_BITS      16

#define MIO_MEM_PULSELEN2_OFFSET        (MIO_INPUT_BASE + 76) /* 89 */
#define MIO_MEM_PULSELEN2_LEN_BITS      16

#define MIO_MEM_PULSELEN3_OFFSET        (MIO_INPUT_BASE + 78) /* 91 */
#define MIO_MEM_PULSELEN3_LEN_BITS      16

#define MIO_MEM_PULSELEN4_OFFSET        (MIO_INPUT_BASE + 80) /* 93 */
#define MIO_MEM_PULSELEN4_LEN_BITS      16

/* AnalogInputMode_1..8 (1 bit each, ByteOff=82, bits 0..7) */
#define MIO_MEM_AIMODE_BYTE_OFFSET      (MIO_INPUT_BASE + 82) /* 95 */

#define MIO_MEM_AIMODE1_OFFSET          MIO_MEM_AIMODE_BYTE_OFFSET
#define MIO_MEM_AIMODE1_BIT             0
#define MIO_MEM_AIMODE1_LEN_BITS        1

#define MIO_MEM_AIMODE2_OFFSET          MIO_MEM_AIMODE_BYTE_OFFSET
#define MIO_MEM_AIMODE2_BIT             1
#define MIO_MEM_AIMODE2_LEN_BITS        1

#define MIO_MEM_AIMODE3_OFFSET          MIO_MEM_AIMODE_BYTE_OFFSET
#define MIO_MEM_AIMODE3_BIT             2
#define MIO_MEM_AIMODE3_LEN_BITS        1

#define MIO_MEM_AIMODE4_OFFSET          MIO_MEM_AIMODE_BYTE_OFFSET
#define MIO_MEM_AIMODE4_BIT             3
#define MIO_MEM_AIMODE4_LEN_BITS        1

#define MIO_MEM_AIMODE5_OFFSET          MIO_MEM_AIMODE_BYTE_OFFSET
#define MIO_MEM_AIMODE5_BIT             4
#define MIO_MEM_AIMODE5_LEN_BITS        1

#define MIO_MEM_AIMODE6_OFFSET          MIO_MEM_AIMODE_BYTE_OFFSET
#define MIO_MEM_AIMODE6_BIT             5
#define MIO_MEM_AIMODE6_LEN_BITS        1

#define MIO_MEM_AIMODE7_OFFSET          MIO_MEM_AIMODE_BYTE_OFFSET
#define MIO_MEM_AIMODE7_BIT             6
#define MIO_MEM_AIMODE7_LEN_BITS        1

#define MIO_MEM_AIMODE8_OFFSET          MIO_MEM_AIMODE_BYTE_OFFSET
#define MIO_MEM_AIMODE8_BIT             7
#define MIO_MEM_AIMODE8_LEN_BITS        1

/* InputLogicLevelVoltage_1..8 (16 bit, ByteOff=83,85,87,89,91,93,95,97) */
#define MIO_MEM_IN_LOGICLVL1_OFFSET     (MIO_INPUT_BASE + 83)
#define MIO_MEM_IN_LOGICLVL1_LEN_BITS   16

#define MIO_MEM_IN_LOGICLVL2_OFFSET     (MIO_INPUT_BASE + 85)
#define MIO_MEM_IN_LOGICLVL2_LEN_BITS   16

#define MIO_MEM_IN_LOGICLVL3_OFFSET     (MIO_INPUT_BASE + 87)
#define MIO_MEM_IN_LOGICLVL3_LEN_BITS   16

#define MIO_MEM_IN_LOGICLVL4_OFFSET     (MIO_INPUT_BASE + 89)
#define MIO_MEM_IN_LOGICLVL4_LEN_BITS   16

#define MIO_MEM_IN_LOGICLVL5_OFFSET     (MIO_INPUT_BASE + 91)
#define MIO_MEM_IN_LOGICLVL5_LEN_BITS   16

#define MIO_MEM_IN_LOGICLVL6_OFFSET     (MIO_INPUT_BASE + 93)
#define MIO_MEM_IN_LOGICLVL6_LEN_BITS   16

#define MIO_MEM_IN_LOGICLVL7_OFFSET     (MIO_INPUT_BASE + 95)
#define MIO_MEM_IN_LOGICLVL7_LEN_BITS   16

#define MIO_MEM_IN_LOGICLVL8_OFFSET     (MIO_INPUT_BASE + 97)
#define MIO_MEM_IN_LOGICLVL8_LEN_BITS   16

/* FilterWindowSize (8 bit, ByteOff=99) */
#define MIO_MEM_FILTER_WINSIZE_OFFSET   (MIO_INPUT_BASE + 99)
#define MIO_MEM_FILTER_WINSIZE_LEN_BITS 8

/* AnalogOutputMode_1..8 (1 bit each, ByteOff=100, bits 0..7) */
#define MIO_MEM_AOMODE_BYTE_OFFSET      (MIO_INPUT_BASE + 100)

#define MIO_MEM_AOMODE1_OFFSET          MIO_MEM_AOMODE_BYTE_OFFSET
#define MIO_MEM_AOMODE1_BIT             0
#define MIO_MEM_AOMODE1_LEN_BITS        1

#define MIO_MEM_AOMODE2_OFFSET          MIO_MEM_AOMODE_BYTE_OFFSET
#define MIO_MEM_AOMODE2_BIT             1
#define MIO_MEM_AOMODE2_LEN_BITS        1

#define MIO_MEM_AOMODE3_OFFSET          MIO_MEM_AOMODE_BYTE_OFFSET
#define MIO_MEM_AOMODE3_BIT             2
#define MIO_MEM_AOMODE3_LEN_BITS        1

#define MIO_MEM_AOMODE4_OFFSET          MIO_MEM_AOMODE_BYTE_OFFSET
#define MIO_MEM_AOMODE4_BIT             3
#define MIO_MEM_AOMODE4_LEN_BITS        1

#define MIO_MEM_AOMODE5_OFFSET          MIO_MEM_AOMODE_BYTE_OFFSET
#define MIO_MEM_AOMODE5_BIT             4
#define MIO_MEM_AOMODE5_LEN_BITS        1

#define MIO_MEM_AOMODE6_OFFSET          MIO_MEM_AOMODE_BYTE_OFFSET
#define MIO_MEM_AOMODE6_BIT             5
#define MIO_MEM_AOMODE6_LEN_BITS        1

#define MIO_MEM_AOMODE7_OFFSET          MIO_MEM_AOMODE_BYTE_OFFSET
#define MIO_MEM_AOMODE7_BIT             6
#define MIO_MEM_AOMODE7_LEN_BITS        1

#define MIO_MEM_AOMODE8_OFFSET          MIO_MEM_AOMODE_BYTE_OFFSET
#define MIO_MEM_AOMODE8_BIT             7
#define MIO_MEM_AOMODE8_LEN_BITS        1

/* OutputLogicLevelVoltage_1..8 (16 bit, ByteOff=101,103,105,107,109,111,113,115) */
#define MIO_MEM_OUT_LOGICLVL1_OFFSET    (MIO_INPUT_BASE + 101)
#define MIO_MEM_OUT_LOGICLVL1_LEN_BITS  16

#define MIO_MEM_OUT_LOGICLVL2_OFFSET    (MIO_INPUT_BASE + 103)
#define MIO_MEM_OUT_LOGICLVL2_LEN_BITS  16

#define MIO_MEM_OUT_LOGICLVL3_OFFSET    (MIO_INPUT_BASE + 105)
#define MIO_MEM_OUT_LOGICLVL3_LEN_BITS  16

#define MIO_MEM_OUT_LOGICLVL4_OFFSET    (MIO_INPUT_BASE + 107)
#define MIO_MEM_OUT_LOGICLVL4_LEN_BITS  16

#define MIO_MEM_OUT_LOGICLVL5_OFFSET    (MIO_INPUT_BASE + 109)
#define MIO_MEM_OUT_LOGICLVL5_LEN_BITS  16

#define MIO_MEM_OUT_LOGICLVL6_OFFSET    (MIO_INPUT_BASE + 111)
#define MIO_MEM_OUT_LOGICLVL6_LEN_BITS  16

#define MIO_MEM_OUT_LOGICLVL7_OFFSET    (MIO_INPUT_BASE + 113)
#define MIO_MEM_OUT_LOGICLVL7_LEN_BITS  16

#define MIO_MEM_OUT_LOGICLVL8_OFFSET    (MIO_INPUT_BASE + 115)
#define MIO_MEM_OUT_LOGICLVL8_LEN_BITS  16

#endif /* REVPI_CONNECT4_MIO_MAP_H */
