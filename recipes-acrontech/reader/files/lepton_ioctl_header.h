#ifndef _LEPTON_IOCTL
#define _LEPTON_IOCTL
#include "lepton_rpmsg_datatype.h"
#define ESME_IOC_MAGIC 'e'

/* send a new config to the MCU and return status fields */
#define ESME_IOC_SET_CONFIG _IOW(ESME_IOC_MAGIC, 1, LeptonCameraConfigData)

/* get the current config to the MCU and return status fields */
#define ESME_IOC_GET_CONFIG _IOR(ESME_IOC_MAGIC, 2, LeptonCameraConfigData)

/* request current config+status from the MCU */
#define ESME_IOC_GET_STATUS _IOR(ESME_IOC_MAGIC, 3, CameraStatus)

#endif /* _LEPTON_IOCTL */
