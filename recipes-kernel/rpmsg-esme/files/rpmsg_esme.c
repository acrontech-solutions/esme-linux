/*
 * Copyright (C) 2015 Freescale Semiconductor, Inc.
 *
 * derived from the omap-rpmsg implementation.
 * Remote processor messaging transport - esme driver
 *
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/virtio.h>
#include <linux/rpmsg.h>
#include <linux/ktime.h>
#include <linux/timekeeping.h>
#include <linux/types.h>

#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>
#include <linux/wait.h>
#include <linux/string.h>
#include <linux/compiler.h>
#include "lepton_ioctl_header.h"

#define DEVICE_NAME "rpmsg_esme"
#define CLASS_NAME "rpmsg_esme_class"

static uint8_t frameData[FRAME_SIZE] = {0x0};
static uint8_t frameDataReadout[FRAME_SIZE] = {0x0};
static FramePacket framePacket;
static uint8_t packetCounter = 0;
static uint8_t frameCounter = 0;
static bool packetResyncNeeded = true;

static ConfigPacket configPacket;

/* Character device related */
static dev_t dev_number;
static struct cdev rpmsg_cdev;
static struct class *rpmsg_class;

static bool data_ready = false;
static DEFINE_MUTEX(rpmsg_frame_mutex);
static DECLARE_WAIT_QUEUE_HEAD(rpmsg_wq);
static DECLARE_COMPLETION(rpmsg_config_got);

struct rpmsg_device *globalRpdev = NULL;

uint32_t time_diff_ms(uint64_t start_ns, uint64_t end_ns)
{
    return (uint32_t)((end_ns - start_ns) / 1000000); // nanoseconds to milliseconds
}

static int rpmsg_esme_cb(struct rpmsg_device *rpdev, void *data, int len,
                         void *priv, u32 src)
{
    int err = 0;
    uint8_t *p = data;
    if (len == sizeof(ConfigPacket) && p[0] == PACKET_M33A55_CONFIG_SEND)
    {
        pr_info("Config recieved\n");
        configPacket = *(ConfigPacket *)data;
        complete(&rpmsg_config_got);
    }
    else if (len == sizeof(FramePacket) && p[0] == PACKET_FRAME)
    {

        framePacket = *(FramePacket *)data;
        if (framePacket.packetCounter == 0 && packetResyncNeeded)
        {
            packetCounter = framePacket.packetCounter;
            frameCounter = framePacket.frameCounter;
            packetResyncNeeded = false;
        }
        else if (framePacket.packetCounter != packetCounter)
        {
            dev_err(&rpdev->dev, "Resync error: packets k %d,mcu %d", packetCounter, framePacket.packetCounter);
            dev_err(&rpdev->dev, "Resync error: frames k %d,mcu %d", frameCounter, framePacket.frameCounter);
            packetResyncNeeded = true;
            return -EIO;
        }

        if (!packetResyncNeeded)
        {
            if (packetCounter < TOTAL_FRAME_FULL_PACKETS)
            {
                memcpy(frameData + packetCounter * FRAME_PACKET_DATA_SIZE,
                       &framePacket.data,
                       FRAME_PACKET_DATA_SIZE);
                packetCounter++;
            }

            else if (packetCounter == TOTAL_FRAME_FULL_PACKETS)
            {
                memcpy(frameData + packetCounter * FRAME_PACKET_DATA_SIZE, &framePacket.data, LAST_FRAME_PACKET_SIZE);
                packetCounter = 0;
                mutex_lock(&rpmsg_frame_mutex);
                memcpy(frameDataReadout, frameData, FRAME_SIZE);
                mutex_unlock(&rpmsg_frame_mutex);
                WRITE_ONCE(data_ready, true);
                wake_up_interruptible(&rpmsg_wq);
                frameCounter++;
            }
            else
            {
                pr_err("Invalid packetCounter %d, resyncing\n", packetCounter);
                packetResyncNeeded = true;
                return 0;
            }
        }
    }
    else
    {
        pr_warn("Received bad packet: %d bytes\n", len);
        return -EINVAL;
    }

    if (err)
        dev_err(&rpdev->dev, "rpmsg_send failed: %d\n", err);

    return err;
}

static ssize_t rpmsg_read(struct file *filp, char __user *buf, size_t count,
                          loff_t *f_pos)
{
    int ret;
    if (count != FRAME_SIZE)
    {
        return -EINVAL; // Invalid argument
    }
    /* Wait for data to be ready */
    if (wait_event_interruptible(rpmsg_wq, data_ready))
        return -ERESTARTSYS;

    mutex_lock(&rpmsg_frame_mutex);

    ret = copy_to_user(buf, frameDataReadout, FRAME_SIZE);
    if (ret == 0)
    {
        ret = FRAME_SIZE;
        WRITE_ONCE(data_ready, false);
    }
    else
    {
        ret = -EFAULT;
    }

    mutex_unlock(&rpmsg_frame_mutex);
    return ret;
}

static long rpmsg_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    int ret;
    ConfigPacket pkt;
    LeptonCameraConfigData cameraConfig;
    CameraStatus cameraStatus;
    pr_info("IOCTL call\n");
    if (globalRpdev == NULL)
        return -EFAULT;
    switch (cmd)
    {

    case ESME_IOC_SET_CONFIG:
        pr_info("Set Config\n");
        /* 1) copy desired config from user */
        if (copy_from_user(&cameraConfig, (void __user *)arg, sizeof(LeptonCameraConfigData)))
            return -EFAULT;

        /* 2) build an RPMSG packet with packetType = request (1) */
        memset(&pkt, 0, sizeof(pkt));
        pkt.packetType = PACKET_A55M33_CONFIG_SET;
        pkt.config = cameraConfig;

        ret = rpmsg_send(globalRpdev->ept, &pkt, sizeof(pkt));
        if (ret)
            return ret;
        pr_info("Set Config sent\n");
        /* 3) wait for the MCU’s reply (PACKET_A55M33_CONFIG_SEND) */
        if (wait_for_completion_interruptible(&rpmsg_config_got))
            return -EINTR;

        cameraConfig = configPacket.config;
        /* 4) copy the full ConfigPacket back to user space */
        if (copy_to_user((void __user *)arg, &cameraConfig, sizeof(LeptonCameraConfigData)))
            return -EFAULT;
        pr_info("Copied to user\n");
        return ret;

    case ESME_IOC_GET_CONFIG:
        pr_info("Get Config\n");
        /* 1) build a “get” request packet */
        memset(&pkt, 0, sizeof(ConfigPacket));
        pkt.packetType = PACKET_A55M33_CONFIG_GET;

        /* 2) re‐init the completion and send */
        reinit_completion(&rpmsg_config_got);
        ret = rpmsg_send(globalRpdev->ept, &pkt, sizeof(pkt));
        if (ret)
            return ret;
        pr_info("Get Config sent\n");
        /* 3) wait for the MCU’s reply (PACKET_A55M33_CONFIG_RECIEVE) */
        if (wait_for_completion_interruptible(&rpmsg_config_got))
            return -EINTR;
        cameraConfig = configPacket.config;
        /* 4) copy the full ConfigPacket back to user space */
        if (copy_to_user((void __user *)arg, &cameraConfig, sizeof(LeptonCameraConfigData)))
            return -EFAULT;
        pr_info("Copied to user\n");
        return 0;

    case ESME_IOC_GET_STATUS:
        pr_info("Get Status\n");
        memset(&pkt, 0, sizeof(ConfigPacket));
        pkt.packetType = PACKET_A55M33_CONFIG_GET;

        /* 2) re‐init the completion and send */
        reinit_completion(&rpmsg_config_got);
        ret = rpmsg_send(globalRpdev->ept, &pkt, sizeof(pkt));
        if (ret)
            return ret;
        pr_info("Get Status sent\n");
        /* 3) wait for the MCU’s reply (PACKET_A55M33_CONFIG_RECIEVE) */
        if (wait_for_completion_interruptible(&rpmsg_config_got))
            return -EINTR;

        cameraStatus.errorReg = configPacket.errorReg;
        cameraStatus.syncCount = configPacket.syncCount;
        cameraStatus.serialNumber = configPacket.serialNumber;

        /* 4) copy the full ConfigPacket back to user space */
        if (copy_to_user((void __user *)arg, &cameraStatus, sizeof(cameraStatus)))
            return -EFAULT;
        pr_info("Copied to user\n");
        return 0;

    default:
        return -EINVAL;
    }
}

static int rpmsg_open(struct inode *inode, struct file *file)
{
    return 0;
}

static int rpmsg_release(struct inode *inode, struct file *file)
{
    return 0;
}

static const struct file_operations rpmsg_fops = {
    .owner = THIS_MODULE,
    .read = rpmsg_read,
    .open = rpmsg_open,
    .release = rpmsg_release,
    .unlocked_ioctl = rpmsg_ioctl,
};

static int rpmsg_esme_probe(struct rpmsg_device *rpdev)
{
    int err;

    /* Allocate character device number */
    err = alloc_chrdev_region(&dev_number, 0, 1, DEVICE_NAME);
    if (err < 0)
    {
        dev_err(&rpdev->dev, "Failed to allocate chrdev region\n");
        return err;
    }

    /* Initialize and add cdev */
    cdev_init(&rpmsg_cdev, &rpmsg_fops);
    rpmsg_cdev.owner = THIS_MODULE;
    err = cdev_add(&rpmsg_cdev, dev_number, 1);
    if (err)
    {
        unregister_chrdev_region(dev_number, 1);
        dev_err(&rpdev->dev, "Failed to add cdev\n");
        return err;
    }

    /* Create class and device node */
    rpmsg_class = class_create(CLASS_NAME);
    if (IS_ERR(rpmsg_class))
    {
        cdev_del(&rpmsg_cdev);
        unregister_chrdev_region(dev_number, 1);
        dev_err(&rpdev->dev, "Failed to create class\n");
        return PTR_ERR(rpmsg_class);
    }
    device_create(rpmsg_class, NULL, dev_number, NULL, DEVICE_NAME);
    globalRpdev = rpdev;
    /* Send initial message */
    memset(&configPacket, 0, sizeof(ConfigPacket));
    configPacket.packetType = PACKET_A55M33_CONFIG_GET;
    err = rpmsg_send(rpdev->ept, &configPacket, sizeof(ConfigPacket));

    pr_info("Sent inital config get\n");
    if (err)
    {
        dev_err(&rpdev->dev, "rpmsg_send failed: %d\n", err);
        return err;
    }

    return 0;
}

static void rpmsg_esme_remove(struct rpmsg_device *rpdev)
{
    device_destroy(rpmsg_class, dev_number);
    class_destroy(rpmsg_class);
    cdev_del(&rpmsg_cdev);
    unregister_chrdev_region(dev_number, 1);

    dev_info(&rpdev->dev, "rpmsg esme driver is removed\n");
}

static struct rpmsg_device_id rpmsg_driver_esme_id_table[] = {
    {.name = "rpmsg-openamp-esme-channel"},
    {.name = "rpmsg-openamp-esme-channel-1"},
    {},
};
MODULE_DEVICE_TABLE(rpmsg, rpmsg_driver_esme_id_table);

static struct rpmsg_driver rpmsg_esme_driver = {
    .drv.name = KBUILD_MODNAME,
    .drv.owner = THIS_MODULE,
    .id_table = rpmsg_driver_esme_id_table,
    .probe = rpmsg_esme_probe,
    .callback = rpmsg_esme_cb,
    .remove = rpmsg_esme_remove,
};

static int __init init(void)
{
    return register_rpmsg_driver(&rpmsg_esme_driver);
}

static void __exit fini(void)
{
    unregister_rpmsg_driver(&rpmsg_esme_driver);
}

module_init(init);
module_exit(fini);

MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_DESCRIPTION("iMX virtio remote processor messaging esme driver with char device read");
MODULE_LICENSE("GPL v2");
