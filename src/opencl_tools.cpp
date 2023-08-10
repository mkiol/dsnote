/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "opencl_tools.hpp"

#include <dlfcn.h>

#include <cstdint>
#include <stdexcept>

#include "logger.hpp"

#define CL_SUCCESS 0
#define CL_DEVICE_NOT_FOUND -1
#define CL_PLATFORM_NAME 0x0902
#define CL_PLATFORM_VENDOR 0x0903
#define CL_DEVICE_TYPE 0x1000
#define CL_DEVICE_NAME 0x102B
#define CL_DEVICE_VENDOR 0x102C
#define CL_DEVICE_TYPE_DEFAULT (1 << 0)
#define CL_DEVICE_TYPE_CPU (1 << 1)
#define CL_DEVICE_TYPE_GPU (1 << 2)
#define CL_DEVICE_TYPE_ACCELERATOR (1 << 3)
#define CL_DEVICE_TYPE_CUSTOM (1 << 4)
#define CL_DEVICE_TYPE_ALL 0xFFFFFFFF

struct opencl_api {
    void* handle = nullptr;
    int32_t (*clGetPlatformIDs)(uint32_t, void*, uint32_t*) = nullptr;
    int32_t (*clGetPlatformInfo)(void*, uint32_t, size_t, void*,
                                 size_t*) = nullptr;
    int32_t (*clGetDeviceIDs)(void*, uint64_t, uint32_t, void*,
                              uint32_t*) = nullptr;
    int32_t (*clGetDeviceInfo)(void*, uint32_t, size_t, void*,
                               size_t*) = nullptr;

    opencl_api() {
        handle = dlopen("libOpenCL.so", RTLD_LAZY);
        if (!handle) {
            LOGD("failed to open libOpenCL.so, so trying libOpenCL.so.1: "
                 << dlerror());
            handle = dlopen("libOpenCL.so.1", RTLD_LAZY);
        }
        if (!handle) {
            LOGD("failed to open libOpenCL.so.1: " << dlerror());
            throw std::runtime_error("failed to open opencl lib");
        }

        clGetPlatformIDs =
            reinterpret_cast<int32_t (*)(uint32_t, void*, uint32_t*)>(
                dlsym(handle, "clGetPlatformIDs"));
        if (!clGetPlatformIDs) {
            LOGD("failed to sym clGetPlatformIDs");
            dlclose(handle);
            throw std::runtime_error("failed to sym clGetPlatformIDs");
        }

        clGetPlatformInfo = reinterpret_cast<int32_t (*)(
            void*, uint32_t, size_t, void*, size_t*)>(
            dlsym(handle, "clGetPlatformInfo"));
        if (!clGetPlatformInfo) {
            LOGD("failed to sym clGetPlatformInfo");
            dlclose(handle);
            throw std::runtime_error("failed to sym clGetPlatformInfo");
        }

        clGetDeviceIDs = reinterpret_cast<int32_t (*)(void*, uint64_t, uint32_t,
                                                      void*, uint32_t*)>(
            dlsym(handle, "clGetDeviceIDs"));
        if (!clGetDeviceIDs) {
            LOGD("failed to sym clGetDeviceIDs");
            dlclose(handle);
            throw std::runtime_error("failed to sym clGetDeviceIDs");
        }

        clGetDeviceInfo = reinterpret_cast<int32_t (*)(void*, uint32_t, size_t,
                                                       void*, size_t*)>(
            dlsym(handle, "clGetDeviceInfo"));
        if (!clGetDeviceInfo) {
            LOGD("failed to sym clGetDeviceInfo");
            dlclose(handle);
            throw std::runtime_error("failed to sym clGetDeviceInfo");
        }
    }

    ~opencl_api() {
        if (handle) dlclose(handle);
    }
};

std::vector<opencl_tools::device> opencl_tools::available_devices() {
    std::vector<opencl_tools::device> devices;

    try {
        opencl_api api;

        static constexpr uint32_t max_items = 16;

        uint32_t n_platforms = 0;
        void* platform_ids[max_items];
        if (auto ret =
                api.clGetPlatformIDs(max_items, platform_ids, &n_platforms);
            ret != CL_SUCCESS) {
            LOGD("clGetPlatformIDs returned: " << ret);
            return devices;
        }

        LOGD("opencl number of platforms: " << n_platforms);

        for (uint32_t i = 0; i < n_platforms; ++i) {
            char pname[128];
            if (auto ret =
                    api.clGetPlatformInfo(platform_ids[i], CL_PLATFORM_NAME,
                                          sizeof(pname), &pname, nullptr);
                ret != CL_SUCCESS) {
                LOGD("clGetPlatformInfo for name returned: " << ret);
                continue;
            }

            char vendor[128];
            if (auto ret =
                    api.clGetPlatformInfo(platform_ids[i], CL_PLATFORM_VENDOR,
                                          sizeof(vendor), &vendor, nullptr);
                ret != CL_SUCCESS) {
                LOGD("clGetPlatformInfo for vendor returned: " << ret);
                continue;
            }

            LOGD("opencl platform: " << i << " name=" << pname
                                     << ", vendor=" << vendor);

            uint32_t n_devices = 0;
            void* device_ids[max_items];
            auto ret = api.clGetDeviceIDs(platform_ids[i], CL_DEVICE_TYPE_ALL,
                                          max_items, device_ids, &n_devices);
            if (ret == CL_DEVICE_NOT_FOUND) {
                n_devices = 0;
            } else if (ret != CL_SUCCESS) {
                LOGD("clGetDeviceIDs returned: " << ret);
                continue;
            }

            LOGD("opencl number of devices: " << n_devices);

            for (uint32_t j = 0; j < n_devices; ++j) {
                char dname[128];
                if (auto ret =
                        api.clGetDeviceInfo(device_ids[j], CL_DEVICE_NAME,
                                            sizeof(dname), &dname, nullptr);
                    ret != CL_SUCCESS) {
                    LOGD("clGetDeviceInfo for name returned: " << ret);
                    continue;
                }

                uint64_t type;
                if (auto ret =
                        api.clGetDeviceInfo(device_ids[j], CL_DEVICE_TYPE,
                                            sizeof(type), &type, nullptr);
                    ret != CL_SUCCESS) {
                    LOGD("clGetDeviceInfo for type returned: " << ret);
                    continue;
                }

                LOGD("opencl device: "
                     << j << " platform name=" << pname
                     << " device name=" << dname << ", types=["
                     << (type & CL_DEVICE_TYPE_DEFAULT ? "DEFAULT, " : "")
                     << (type & CL_DEVICE_TYPE_GPU ? "GPU, " : "")
                     << (type & CL_DEVICE_TYPE_CPU ? "CPU, " : "")
                     << (type & CL_DEVICE_TYPE_ACCELERATOR ? "ACCELERATOR, "
                                                           : "")
                     << (type & CL_DEVICE_TYPE_CUSTOM ? "CUSTOM, " : "")
                     << "]");

                if (type & CL_DEVICE_TYPE_GPU) {
                    devices.push_back({pname, dname});
                }
            }
        }
    } catch (const std::runtime_error& err) {
    }

    return devices;
}
