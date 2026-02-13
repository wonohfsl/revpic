#ifndef RO_H
#define RO_H

#include <stdint.h>

/* ============================================================
   RO OFFSETS (from piControl0)
   ============================================================ */

/* Relay Outputs (RO1–RO4) */
#define RO1_OFFSET   114
#define RO1_BIT      0

#define RO2_OFFSET   114
#define RO2_BIT      1

#define RO3_OFFSET   114
#define RO3_BIT      2

#define RO4_OFFSET   114
#define RO4_BIT      3

/* ============================================================
   FUNCTION PROTOTYPES
   ============================================================ */

int ro_init(void);
int ro_set(int ch, int value);

#endif
