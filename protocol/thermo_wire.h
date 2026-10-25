#ifndef THERMO_WIRE_H
#define THERMO_WIRE_H

/* Version 2 uses explicit byte offsets, never native structure packing. */
#define THERMO_REQUEST_SIZE 20
#define THERMO_RESPONSE_SIZE 32
#define THERMO_TOKEN_SIZE 16
#define THERMO_CAPABILITY_REQUEST 0x22
#define THERMO_REQUEST_MAGIC "THQ2"
#define THERMO_RESPONSE_MAGIC "THR2"

#endif
