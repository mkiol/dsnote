/* Copyright (C) 2023 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "gpu_tools.hpp"

#include <dlfcn.h>
#include <fmt/format.h>

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <utility>

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

namespace gpu_tools {
enum cudaHipError { cudaHipSuccess = 0 };

struct cudaHipDeviceProp {
    char name[256] = {};
    char other_props[1024] = {};
};

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
        if (!handle) handle = dlopen("libOpenCL.so.1", RTLD_LAZY);
        if (!handle) {
            LOGW("failed to open opencl lib: " << dlerror());
            throw std::runtime_error("failed to open opencl lib");
        }

        clGetPlatformIDs =
            reinterpret_cast<int32_t (*)(uint32_t, void*, uint32_t*)>(
                dlsym(handle, "clGetPlatformIDs"));
        if (!clGetPlatformIDs) {
            LOGW("failed to sym clGetPlatformIDs");
            dlclose(handle);
            throw std::runtime_error("failed to sym clGetPlatformIDs");
        }

        clGetPlatformInfo = reinterpret_cast<int32_t (*)(
            void*, uint32_t, size_t, void*, size_t*)>(
            dlsym(handle, "clGetPlatformInfo"));
        if (!clGetPlatformInfo) {
            LOGW("failed to sym clGetPlatformInfo");
            dlclose(handle);
            throw std::runtime_error("failed to sym clGetPlatformInfo");
        }

        clGetDeviceIDs = reinterpret_cast<int32_t (*)(void*, uint64_t, uint32_t,
                                                      void*, uint32_t*)>(
            dlsym(handle, "clGetDeviceIDs"));
        if (!clGetDeviceIDs) {
            LOGW("failed to sym clGetDeviceIDs");
            dlclose(handle);
            throw std::runtime_error("failed to sym clGetDeviceIDs");
        }

        clGetDeviceInfo = reinterpret_cast<int32_t (*)(void*, uint32_t, size_t,
                                                       void*, size_t*)>(
            dlsym(handle, "clGetDeviceInfo"));
        if (!clGetDeviceInfo) {
            LOGW("failed to sym clGetDeviceInfo");
            dlclose(handle);
            throw std::runtime_error("failed to sym clGetDeviceInfo");
        }
    }

    ~opencl_api() {
        if (handle) {
            dlclose(handle);
        }
    }
};

struct cuda_api {
    void* handle = nullptr;
    int (*cudaGetDeviceCount)(int*) = nullptr;
    int (*cudaGetDeviceProperties)(cudaHipDeviceProp*, int) = nullptr;
    int (*cudaRuntimeGetVersion)(int*) = nullptr;
    int (*cudaDriverGetVersion)(int*) = nullptr;

    cuda_api() {
        handle = dlopen("libcudart.so", RTLD_LAZY);
        if (!handle) {
            LOGW("failed to open cudart lib: " << dlerror());
            throw std::runtime_error("failed to open cudart lib");
        }

        cudaGetDeviceCount = reinterpret_cast<int (*)(int*)>(
            dlsym(handle, "cudaGetDeviceCount"));
        if (!cudaGetDeviceCount) {
            LOGW("failed to sym cudaGetDeviceCount");
            dlclose(handle);
            throw std::runtime_error("failed to sym cudaGetDeviceCount");
        }

        cudaGetDeviceProperties =
            reinterpret_cast<int (*)(cudaHipDeviceProp*, int)>(
                dlsym(handle, "cudaGetDeviceProperties"));
        if (!cudaGetDeviceProperties) {
            LOGW("failed to sym cudaGetDeviceProperties");
            dlclose(handle);
            throw std::runtime_error("failed to sym cudaGetDeviceProperties");
        }

        cudaRuntimeGetVersion = reinterpret_cast<int (*)(int*)>(
            dlsym(handle, "cudaRuntimeGetVersion"));
        if (!cudaRuntimeGetVersion) {
            LOGW("failed to sym cudaRuntimeGetVersion");
            dlclose(handle);
            throw std::runtime_error("failed to sym cudaRuntimeGetVersion");
        }

        cudaDriverGetVersion = reinterpret_cast<int (*)(int*)>(
            dlsym(handle, "cudaDriverGetVersion"));
        if (!cudaRuntimeGetVersion) {
            LOGW("failed to sym cudaDriverGetVersion");
            dlclose(handle);
            throw std::runtime_error("failed to sym cudaDriverGetVersion");
        }
    }

    ~cuda_api() {
        if (handle) {
            dlclose(handle);
        }
    }
};

struct hip_api {
    void* handle = nullptr;
    int (*hipGetDeviceCount)(int*) = nullptr;
    int (*hipGetDeviceProperties)(cudaHipDeviceProp*, int) = nullptr;
    int (*hipRuntimeGetVersion)(int*) = nullptr;
    int (*hipDriverGetVersion)(int*) = nullptr;

    hip_api() {
        handle = dlopen("libamdhip64.so", RTLD_LAZY);
        if (!handle) {
            LOGW("failed to open hip lib: " << dlerror());
            throw std::runtime_error("failed to open hip lib");
        }

        hipGetDeviceCount =
            reinterpret_cast<int (*)(int*)>(dlsym(handle, "hipGetDeviceCount"));
        if (!hipGetDeviceCount) {
            LOGW("failed to sym hipGetDeviceCount");
            dlclose(handle);
            throw std::runtime_error("failed to sym hipGetDeviceCount");
        }

        hipGetDeviceProperties =
            reinterpret_cast<int (*)(cudaHipDeviceProp*, int)>(
                dlsym(handle, "hipGetDeviceProperties"));
        if (!hipGetDeviceProperties) {
            LOGW("failed to sym hipGetDeviceProperties");
            dlclose(handle);
            throw std::runtime_error("failed to sym hipGetDeviceProperties");
        }

        hipRuntimeGetVersion = reinterpret_cast<int (*)(int*)>(
            dlsym(handle, "hipRuntimeGetVersion"));
        if (!hipRuntimeGetVersion) {
            LOGW("failed to sym hipRuntimeGetVersion");
            dlclose(handle);
            throw std::runtime_error("failed to sym hipRuntimeGetVersion");
        }

        hipDriverGetVersion = reinterpret_cast<int (*)(int*)>(
            dlsym(handle, "hipDriverGetVersion"));
        if (!hipRuntimeGetVersion) {
            LOGW("failed to sym hipDriverGetVersion");
            dlclose(handle);
            throw std::runtime_error("failed to sym hipDriverGetVersion");
        }
    }

    ~hip_api() {
        if (handle) {
            dlclose(handle);
        }
    }
};

std::vector<gpu_tools::device> available_devices(bool cuda, bool hip,
                                                 bool opencl,
                                                 bool opencl_always) {
    std::vector<gpu_tools::device> devices;

    if (cuda) add_cuda_devices(devices);
    if (hip) add_hip_devices(devices);
    if (opencl && (opencl_always || devices.empty()))
        add_opencl_devices(devices);

    return devices;
}

void add_cuda_devices(std::vector<device>& devices) {
    LOGD("scanning for cuda devices");

    try {
        cuda_api api;

        int dr_ver = 0, rt_ver = 0;
        api.cudaDriverGetVersion(&dr_ver);
        api.cudaRuntimeGetVersion(&rt_ver);

        LOGD("cuda version: driver=" << dr_ver << ", runtime=" << rt_ver);

        int device_count = 0;
        if (auto ret = api.cudaGetDeviceCount(&device_count);
            ret != cudaHipSuccess) {
            LOGW("cudaGetDeviceCount returned: " << ret);
            return;
        }

        LOGD("cuda number of devices: " << device_count);

        devices.reserve(devices.size() + device_count);

        for (int i = 0; i < device_count; ++i) {
            cudaHipDeviceProp props;
            if (auto ret = api.cudaGetDeviceProperties(&props, i);
                ret != cudaHipSuccess)
                continue;
            LOGD("cuda device: " << i << ", name=" << props.name);
            devices.push_back({/*id=*/static_cast<uint32_t>(i), api_t::cuda,
                               /*name=*/props.name,
                               /*platform_name=*/{}});
        }
    } catch ([[maybe_unused]] const std::runtime_error& err) {
    }
}

void add_hip_devices(std::vector<device>& devices) {
    LOGD("scanning for hip devices");

    try {
        hip_api api;

        int dr_ver = 0, rt_ver = 0;
        api.hipDriverGetVersion(&dr_ver);
        api.hipRuntimeGetVersion(&rt_ver);

        LOGD("hip version: driver=" << dr_ver << ", runtime=" << rt_ver);

        int device_count = 0;
        if (auto ret = api.hipGetDeviceCount(&device_count);
            ret != cudaHipSuccess) {
            LOGW("hipGetDeviceCount returned: " << ret);
            return;
        }

        LOGD("hip number of devices: " << device_count);

        devices.reserve(devices.size() + device_count);

        for (int i = 0; i < device_count; ++i) {
            cudaHipDeviceProp props;
            if (auto ret = api.hipGetDeviceProperties(&props, i);
                ret != cudaHipSuccess)
                continue;
            LOGD("hip device: " << i << ", name=" << props.name);
            devices.push_back({/*id=*/static_cast<uint32_t>(i), api_t::rocm,
                               /*name=*/props.name,
                               /*platform_name=*/{}});
        }
    } catch ([[maybe_unused]] const std::runtime_error& err) {
    }
}

void add_opencl_devices(std::vector<device>& devices) {
    LOGD("scanning for opencl devices");

    try {
        opencl_api api;

        static constexpr uint32_t max_items = 16;
        static constexpr size_t max_name_size = 512;

        uint32_t n_platforms = 0;
        void* platform_ids[max_items];
        if (auto ret =
                api.clGetPlatformIDs(max_items, platform_ids, &n_platforms);
            ret != CL_SUCCESS) {
            LOGW("clGetPlatformIDs returned: " << ret);
            return;
        }

        LOGD("opencl number of platforms: " << n_platforms);

        std::vector<std::pair<std::string, std::vector<device>>>
            platforms_with_devices;
        platforms_with_devices.reserve(n_platforms);

        for (uint32_t i = 0; i < n_platforms; ++i) {
            char pname[max_name_size];
            if (auto ret =
                    api.clGetPlatformInfo(platform_ids[i], CL_PLATFORM_NAME,
                                          max_name_size, &pname, nullptr);
                ret != CL_SUCCESS) {
                LOGW("clGetPlatformInfo for name returned: " << ret);
                continue;
            }

            char vendor[max_name_size];
            if (auto ret =
                    api.clGetPlatformInfo(platform_ids[i], CL_PLATFORM_VENDOR,
                                          max_name_size, &vendor, nullptr);
                ret != CL_SUCCESS) {
                LOGW("clGetPlatformInfo for vendor returned: " << ret);
                continue;
            }

            LOGD("opencl platform: " << i << ", name=" << pname
                                     << ", vendor=" << vendor);

            uint32_t n_devices = 0;
            void* device_ids[max_items];
            auto ret = api.clGetDeviceIDs(platform_ids[i], CL_DEVICE_TYPE_ALL,
                                          max_items, device_ids, &n_devices);
            if (ret == CL_DEVICE_NOT_FOUND) {
                n_devices = 0;
            } else if (ret != CL_SUCCESS) {
                LOGW("clGetDeviceIDs returned: " << ret);
                continue;
            }

            LOGD("opencl number of devices: " << n_devices);

            std::pair<std::string, std::vector<device>> devices_in_platform;
            devices_in_platform.first = pname;

            for (uint32_t j = 0; j < n_devices; ++j) {
                char dname[max_name_size];
                if (auto ret =
                        api.clGetDeviceInfo(device_ids[j], CL_DEVICE_NAME,
                                            max_name_size, &dname, nullptr);
                    ret != CL_SUCCESS) {
                    LOGW("clGetDeviceInfo for name returned: " << ret);
                    continue;
                }

                uint64_t type;
                if (auto ret =
                        api.clGetDeviceInfo(device_ids[j], CL_DEVICE_TYPE,
                                            sizeof(type), &type, nullptr);
                    ret != CL_SUCCESS) {
                    LOGW("clGetDeviceInfo for type returned: " << ret);
                    continue;
                }

                LOGD("opencl device: "
                     << j << ", platform name=" << pname
                     << ", device name=" << dname << ", types=["
                     << (type & CL_DEVICE_TYPE_DEFAULT ? "DEFAULT, " : "")
                     << (type & CL_DEVICE_TYPE_GPU ? "GPU, " : "")
                     << (type & CL_DEVICE_TYPE_CPU ? "CPU, " : "")
                     << (type & CL_DEVICE_TYPE_ACCELERATOR ? "ACCELERATOR, "
                                                           : "")
                     << (type & CL_DEVICE_TYPE_CUSTOM ? "CUSTOM, " : "")
                     << "]");

                if (type & CL_DEVICE_TYPE_GPU) {
                    devices_in_platform.second.push_back(
                        {/*id=*/i, api_t::opencl, /*name=*/dname,
                         /*platform_name=*/pname});
                }
            }

            platforms_with_devices.push_back(std::move(devices_in_platform));
        }

        std::sort(
            platforms_with_devices.begin(), platforms_with_devices.end(),
            [](const auto& p1, const auto& p2) { return p1.first < p2.first; });

        std::for_each(platforms_with_devices.begin(),
                      platforms_with_devices.end(), [&devices](auto& platform) {
                          std::for_each(
                              platform.second.begin(), platform.second.end(),
                              [&devices](auto& device) {
                                  devices.push_back(std::move(device));
                              });
                      });
    } catch ([[maybe_unused]] const std::runtime_error& err) {
    }
}

static bool has_lib(const char* name) {
    auto handle = dlopen(name, RTLD_LAZY);
    if (!handle) {
        LOGW("failed to open " << name << ": " << dlerror());
        return false;
    }

    dlclose(handle);

    return true;
}

bool has_cuda() { return has_lib("libcudart.so"); }

bool has_clblast() { return has_lib("libclblast.so"); }

bool has_cudnn() {
    if (has_lib("libcudnn.so")) return true;
    return has_lib("libcudnn.so.8");
}
}  // namespace gpu_tools
