#include <iostream>
#include <fstream>
#include <thread>
#include <atomic>
#include <chrono>
#include <csignal>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <sys/ioctl.h>

// include your datatype and ioctl definitions
#include "lepton_ioctl_header.h"

constexpr const char *DEVICE_PATH = "/dev/rpmsg_esme";
constexpr const char *OUTPUT_FILE_1 = "/root/first.nvid";
constexpr const char *OUTPUT_FILE_2 = "/root/second.nvid";
constexpr size_t FRAME_SIZE_CONST = FRAME_SIZE;
constexpr int READ_TIMEOUT_MS = 2000;
constexpr int VIDEO_DURATION_S = 30;

std::atomic<bool> running{true};
std::atomic<size_t> frameCount{0};

void printConfig(const LeptonCameraConfigData &cfg)
{
    std::cout << "CameraConfig:\n"
              << "  agcEn:            " << (int)cfg.agcEn << "\n"
              << "  agcPolicy:        " << (int)cfg.agcPolicy << "\n"
              << "  gainMode:         " << (int)cfg.gainMode << "\n"
              << "  gpioMode:         " << (int)cfg.gpioMode << "\n"
              << "  videoOutputSource:" << (int)cfg.videoOutputSource << "\n"
              << "  pixelNoiseFilter: " << (int)cfg.pixelNoiseFilter << "\n"
              << "  columnNoiseFilter:" << (int)cfg.columnNoiseFilter << "\n"
              << "  temporalFilter:   " << (int)cfg.temporalFilter << "\n";
}

void printStatus(const CameraStatus &st)
{
    std::cout << "CameraStatus:\n"
              << "  errorReg:     " << (int)st.errorReg << "\n"
              << "  syncCount:    " << st.syncCount << "\n"
              << "  serialNumber: " << st.serialNumber << "\n";
}

int do_ioctl(int fd, unsigned long cmd, void *arg)
{
    int ret = ioctl(fd, cmd, arg);
    if (ret < 0)
    {
        std::cerr << "ioctl(" << std::hex << cmd << std::dec
                  << ") failed: " << strerror(errno) << "\n";
    }
    return ret;
}

bool recordLoop(int dev_fd, std::ofstream &outputFile, int duration_s)
{
    uint8_t frame[FRAME_SIZE_CONST];
    frameCount = 0;
    auto start = std::chrono::steady_clock::now();
    auto nextTimeout = start;
    while (running)
    {
        auto now = std::chrono::steady_clock::now();
        if (now - start >= std::chrono::seconds(duration_s))
            break;

        ssize_t bytesRead = read(dev_fd, frame, FRAME_SIZE_CONST);
        if (bytesRead == FRAME_SIZE_CONST)
        {
            outputFile.write(reinterpret_cast<char *>(frame), FRAME_SIZE_CONST);
            ++frameCount;
            nextTimeout = now;
        }
        else if (bytesRead < 0)
        {
            std::cerr << "\n[ERROR] Read error: " << strerror(errno) << "\n";
            return false;
        }
        else
        {
            std::cerr << "\n[WARN] Incomplete frame read: " << bytesRead << " bytes\n";
        }

        now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - nextTimeout).count() > READ_TIMEOUT_MS)
        {
            std::cerr << "\n[ERROR] No new frame in " << READ_TIMEOUT_MS << "ms. Aborting.\n";
            return false;
        }
    }
    std::cout << "\nRecording stopped. Total frames: " << frameCount << "\n";
    return true;
}

int main()
{
    int dev_fd = open(DEVICE_PATH, O_RDONLY);
    if (dev_fd < 0)
    {
        std::cerr << "Failed to open " << DEVICE_PATH << ": " << strerror(errno) << "\n";
        return 1;
    }

    LeptonCameraConfigData cfg;
    CameraStatus st;

    // Initial config and status
    std::cout << "\n=== Initial Camera Config and Status ===\n";
    if (do_ioctl(dev_fd, ESME_IOC_GET_CONFIG, &cfg) < 0)
        return 1;
    if (do_ioctl(dev_fd, ESME_IOC_GET_STATUS, &st) < 0)
        return 1;
    printConfig(cfg);
    std::cout << "\n\n";
    printStatus(st);

    // Record first video
    std::ofstream out1(OUTPUT_FILE_1, std::ios::binary);
    if (!out1)
    {
        std::cerr << "Failed to open " << OUTPUT_FILE_1 << "\n";
        close(dev_fd);
        return 1;
    }
    std::cout << "\n=== Starting first 30s capture (first.nvid) ===\n";
    if (!recordLoop(dev_fd, out1, VIDEO_DURATION_S))
    {
        close(dev_fd);
        return 1;
    }
    out1.close();

    // Modify config
    cfg.pixelNoiseFilter = true;
    cfg.columnNoiseFilter = true;
    std::cout << "\nSet new config\n";
    if (do_ioctl(dev_fd, ESME_IOC_SET_CONFIG, &cfg) < 0)
        return 1;

    // Give some time for the new settings to apply
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    // Re-read config and status
    std::cout << "\n=== Updated Camera Config and Status ===\n";
    if (do_ioctl(dev_fd, ESME_IOC_GET_CONFIG, &cfg) < 0)
        return 1;
    if (do_ioctl(dev_fd, ESME_IOC_GET_STATUS, &st) < 0)
        return 1;
    printConfig(cfg);
    printStatus(st);

    // Record second video
    std::ofstream out2(OUTPUT_FILE_2, std::ios::binary);
    if (!out2)
    {
        std::cerr << "Failed to open " << OUTPUT_FILE_2 << "\n";
        close(dev_fd);
        return 1;
    }
    std::cout << "\n=== Starting second 30s capture (second.nvid) ===\n";
    if (!recordLoop(dev_fd, out2, VIDEO_DURATION_S))
    {
        close(dev_fd);
        return 1;
    }
    out2.close();

    std::cout << "\nDone.\n";
    close(dev_fd);
    return 0;
}
