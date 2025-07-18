#ifndef _LEPTON_RPMSG_DATATYPE_H
#define _LEPTON_RPMSG_DATATYPE_H

#define FRAME_SIZE 38489
#define MAX_TRANSFER_SIZE 496
#define FRAME_PACKET_DATA_SIZE 493 // sizeof not frame data values in a packet
#define TOTAL_FRAME_FULL_PACKETS (FRAME_SIZE / FRAME_PACKET_DATA_SIZE)
#define LAST_FRAME_PACKET_SIZE (FRAME_SIZE % FRAME_PACKET_DATA_SIZE)
#define TOTAL_FRAME_PACKETS ((LAST_FRAME_PACKET_SIZE == 0) ? TOTAL_FRAME_FULL_PACKETS : (TOTAL_FRAME_FULL_PACKETS + 1))

#define PACKET_FRAME 0              // payload is a FramePacket M33 -> A55
#define PACKET_M33A55_CONFIG_SEND 1 // “config request” from M33 → A55
#define PACKET_A55M33_CONFIG_SET 2  // “config reply” from A55 → M33
#define PACKET_A55M33_CONFIG_GET 3  // “config reply” from A55 → M33

typedef struct __attribute__((packed))
{
    bool agcEn;
    uint8_t agcPolicy;
    uint8_t gainMode;
    uint8_t gpioMode;
    uint8_t videoOutputSource;
    bool pixelNoiseFilter;
    bool columnNoiseFilter;
    bool temporalFilter;

} LeptonCameraConfigData;

typedef struct __attribute__((packed)) FramePacket
{
    uint8_t packetType;
    uint8_t packetCounter;
    uint8_t frameCounter;
    uint8_t data[FRAME_PACKET_DATA_SIZE];
} FramePacket;

typedef struct __attribute__((packed))
{
    uint8_t packetType;
    uint8_t errorReg;
    uint64_t serialNumber;
    uint16_t syncCount;
    LeptonCameraConfigData config;
} ConfigPacket;

/* user‑space sees only the status fields on return */
typedef struct __attribute__((packed))
{
    uint8_t errorReg;
    uint16_t syncCount;
    uint64_t serialNumber;
} CameraStatus;

#endif /* _LEPTON_RPMSG_DATATYPE_H */