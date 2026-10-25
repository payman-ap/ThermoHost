#include "tusb.h"

/* Arbitrary vendor request code, advertised by the BOS descriptor below. */
#define THERMO_MS_OS_REQUEST 0x20
#define THERMO_MS_OS_LENGTH  0x00A2
#define THERMO_BOS_LENGTH \
    (TUD_BOS_DESC_LEN + TUD_BOS_MICROSOFT_OS_DESC_LEN)

/* Windows discovers this capability through the standard BOS request. */
static const uint8_t bos_descriptor[] = {
    TUD_BOS_DESCRIPTOR(THERMO_BOS_LENGTH, 1),
    TUD_BOS_MS_OS_20_DESCRIPTOR(THERMO_MS_OS_LENGTH, THERMO_MS_OS_REQUEST)
};

/* Device interface GUID: {D026972A-922E-455F-A035-B08D3E383FE4}
   Keep this stable; the Windows DLL will enumerate this interface GUID. */
static const uint8_t ms_os_20_descriptor[] = {
    /* Set header: 10 bytes, Windows 8.1 or newer, total set length 162. */
    U16_TO_U8S_LE(0x000A),
    U16_TO_U8S_LE(MS_OS_20_SET_HEADER_DESCRIPTOR),
    U32_TO_U8S_LE(0x06030000),
    U16_TO_U8S_LE(THERMO_MS_OS_LENGTH),

    /* This is a single-interface device, so apply properties at device scope.
       Compatible ID: WINUSB padded to eight bytes, empty sub-compatible ID. */
    U16_TO_U8S_LE(0x0014),
    U16_TO_U8S_LE(MS_OS_20_FEATURE_COMPATBLE_ID),
    'W', 'I', 'N', 'U', 'S', 'B', 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

    /* Registry property: length 132, REG_MULTI_SZ, UTF-16LE property name. */
    U16_TO_U8S_LE(0x0084),
    U16_TO_U8S_LE(MS_OS_20_FEATURE_REG_PROPERTY),
    U16_TO_U8S_LE(0x0007),
    U16_TO_U8S_LE(0x002A),

    /* "DeviceInterfaceGUIDs" with one UTF-16 null terminator: 42 bytes. */
    0x44, 0x00, 0x65, 0x00, 0x76, 0x00, 0x69, 0x00, 0x63, 0x00, 0x65, 0x00,
    0x49, 0x00, 0x6E, 0x00, 0x74, 0x00, 0x65, 0x00, 0x72, 0x00, 0x66, 0x00,
    0x61, 0x00, 0x63, 0x00, 0x65, 0x00, 0x47, 0x00, 0x55, 0x00, 0x49, 0x00,
    0x44, 0x00, 0x73, 0x00, 0x00, 0x00,

    /* One GUID plus the REG_MULTI_SZ double terminator: 80 bytes. */
    U16_TO_U8S_LE(0x0050),
    0x7B, 0x00, 0x44, 0x00, 0x30, 0x00, 0x32, 0x00, 0x36, 0x00, 0x39, 0x00,
    0x37, 0x00, 0x32, 0x00, 0x41, 0x00, 0x2D, 0x00, 0x39, 0x00, 0x32, 0x00,
    0x32, 0x00, 0x45, 0x00, 0x2D, 0x00, 0x34, 0x00, 0x35, 0x00, 0x35, 0x00,
    0x46, 0x00, 0x2D, 0x00, 0x41, 0x00, 0x30, 0x00, 0x33, 0x00, 0x35, 0x00,
    0x2D, 0x00, 0x42, 0x00, 0x30, 0x00, 0x38, 0x00, 0x44, 0x00, 0x33, 0x00,
    0x45, 0x00, 0x33, 0x00, 0x38, 0x00, 0x33, 0x00, 0x46, 0x00, 0x45, 0x00,
    0x34, 0x00, 0x7D, 0x00, 0x00, 0x00, 0x00, 0x00,
};

/* Catch descriptor-length mistakes at compile time. */
TU_VERIFY_STATIC(sizeof(bos_descriptor) == THERMO_BOS_LENGTH,
                 "Incorrect BOS descriptor length");
TU_VERIFY_STATIC(sizeof(ms_os_20_descriptor) == THERMO_MS_OS_LENGTH,
                 "Incorrect Microsoft OS 2.0 descriptor length");

/* TinyUSB requests this persistent buffer during enumeration. */
const uint8_t* tud_descriptor_bos_cb(void)
{
    return bos_descriptor;
}

/* Answer only the device-to-host Microsoft OS 2.0 descriptor request. */
bool tud_vendor_control_xfer_cb(
    uint8_t rhport,
    uint8_t stage,
    const tusb_control_request_t* request)
{
    /* TinyUSB handles data transfer and acknowledgement after setup. */
    if (stage != CONTROL_STAGE_SETUP) {
        return true;
    }

#ifdef THERMO_FRESH_READS
    /* Advertise the fresh-read protocol without changing driver descriptors. */
    if (request->bmRequestType == 0xC0 && request->bRequest == 0x22 &&
        request->wIndex == 0 && request->wValue == 0) {
        static const uint8_t capabilities[] = {'T', 'H', 2, 20, 32};
        return tud_control_xfer(rhport, request, (void*)capabilities,
                                sizeof(capabilities));
    }
#endif

    /* 0xC0 = device-to-host, vendor request, device recipient.
       Index 7 selects the descriptor set; value zero is required. */
    if ((request->bmRequestType != 0xC0) ||
        (request->bRequest != THERMO_MS_OS_REQUEST) ||
        (request->wIndex != 0x0007) ||
        (request->wValue != 0x0000)) {
        return false;
    }

    /* TinyUSB limits the response to the host's requested length.
       Its API takes void*, although this IN transfer reads constant data. */
    return tud_control_xfer(
        rhport,
        request,
        (void*)ms_os_20_descriptor,
        sizeof(ms_os_20_descriptor));
}
