/* Copyright (C) 2023-2024 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "gpu_tools.hpp"

#include <dlfcn.h>
#include <fmt/core.h>
#include <sys/stat.h>

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

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
enum cudaHipError { cudaHipSuccess = 0, cudaHipUnknownError = 999 };

struct cudaDeviceProp {
    char name[256] = {};
    char other_props[1024] = {};
};

enum CUresult { CUDA_SUCCESS = 0, CUDA_ERROR_UNKNOWN = 999 };
enum CUdevice_attribute {
    CU_DEVICE_ATTRIBUTE_COMPUTE_CAPABILITY_MAJOR = 75,
    CU_DEVICE_ATTRIBUTE_COMPUTE_CAPABILITY_MINOR = 76
};

struct hipDeviceArch {
    unsigned hasGlobalInt32Atomics : 1;
    unsigned hasGlobalFloatAtomicExch : 1;
    unsigned hasSharedInt32Atomics : 1;
    unsigned hasSharedFloatAtomicExch : 1;
    unsigned hasFloatAtomicAdd : 1;
    unsigned hasGlobalInt64Atomics : 1;
    unsigned hasSharedInt64Atomics : 1;
    unsigned hasDoubles : 1;
    unsigned hasWarpVote : 1;
    unsigned hasWarpBallot : 1;
    unsigned hasWarpShuffle : 1;
    unsigned hasFunnelShift : 1;
    unsigned hasThreadFenceSystem : 1;
    unsigned hasSyncThreadsExt : 1;
    unsigned hasSurfaceFuncs : 1;
    unsigned has3dGrid : 1;
    unsigned hasDynamicParallelism : 1;
};

struct hipDeviceProp {
    char name[256];
    size_t totalGlobalMem;
    size_t sharedMemPerBlock;
    int regsPerBlock;
    int warpSize;
    int maxThreadsPerBlock;
    int maxThreadsDim[3];
    int maxGridSize[3];
    int clockRate;
    int memoryClockRate;
    int memoryBusWidth;
    size_t totalConstMem;
    int major;
    int minor;
    int multiProcessorCount;
    int l2CacheSize;
    int maxThreadsPerMultiProcessor;
    int computeMode;
    int clockInstructionRate;
    hipDeviceArch arch;
    int concurrentKernels;
    int pciDomainID;
    int pciBusID;
    int pciDeviceID;
    size_t maxSharedMemoryPerMultiProcessor;
    int isMultiGpuBoard;
    int canMapHostMemory;
    int gcnArch;
    char gcnArchName[256];
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

struct cuda_dev_api {
    void* handle = nullptr;
    CUresult (*cuInit)(unsigned int Flags);
    CUresult (*cuGetErrorName)(CUresult error, const char** pStr);
    CUresult (*cuDriverGetVersion)(int* driverVersion);
    CUresult (*cuDeviceGetCount)(int* count) = nullptr;
    CUresult (*cuDeviceGet)(int* dev, int ordinal) = nullptr;
    CUresult (*cuDeviceGetName)(char* name, int len, int dev) = nullptr;
    CUresult (*cuDeviceGetAttribute)(int* pi, CUdevice_attribute attrib,
                                     int dev) = nullptr;

    cuda_dev_api() {
        handle = dlopen("libcuda.so", RTLD_LAZY);
        if (!handle) {
            LOGW("failed to open cuda lib: " << dlerror());
            throw std::runtime_error("failed to open cuda lib");
        }

        cuInit = reinterpret_cast<decltype(cuInit)>(dlsym(handle, "cuInit"));
        if (!cuInit) {
            LOGW("failed to sym cuInit");
            dlclose(handle);
            throw std::runtime_error("failed to sym cuInit");
        }

        cuGetErrorName = reinterpret_cast<decltype(cuGetErrorName)>(
            dlsym(handle, "cuGetErrorName"));
        if (!cuGetErrorName) {
            LOGW("failed to sym cuGetErrorName");
            dlclose(handle);
            throw std::runtime_error("failed to sym cuGetErrorName");
        }

        cuDriverGetVersion = reinterpret_cast<decltype(cuDriverGetVersion)>(
            dlsym(handle, "cuDriverGetVersion"));
        if (!cuDriverGetVersion) {
            LOGW("failed to sym cuDriverGetVersion");
            dlclose(handle);
            throw std::runtime_error("failed to sym cuDriverGetVersion");
        }

        cuDeviceGetCount = reinterpret_cast<decltype(cuDeviceGetCount)>(
            dlsym(handle, "cuDeviceGetCount"));
        if (!cuDeviceGetCount) {
            LOGW("failed to sym cuDeviceGetCount");
            dlclose(handle);
            throw std::runtime_error("failed to sym cuDeviceGetCount");
        }

        cuDeviceGet = reinterpret_cast<decltype(cuDeviceGet)>(
            dlsym(handle, "cuDeviceGet"));
        if (!cuDeviceGet) {
            LOGW("failed to sym cuDeviceGet");
            dlclose(handle);
            throw std::runtime_error("failed to sym cuDeviceGet");
        }

        cuDeviceGetName = reinterpret_cast<decltype(cuDeviceGetName)>(
            dlsym(handle, "cuDeviceGetName"));
        if (!cuDeviceGetName) {
            LOGW("failed to sym cuDeviceGetName");
            dlclose(handle);
            throw std::runtime_error("failed to sym cuDeviceGetName");
        }

        cuDeviceGetAttribute = reinterpret_cast<decltype(cuDeviceGetAttribute)>(
            dlsym(handle, "cuDeviceGetAttribute"));
        if (!cuDeviceGetAttribute) {
            LOGW("failed to sym cuDeviceGetAttribute");
            dlclose(handle);
            throw std::runtime_error("failed to sym cuDeviceGetAttribute");
        }
    }

    ~cuda_dev_api() {
        if (handle) {
            dlclose(handle);
        }
    }
};

struct cuda_runtime_api {
    void* handle = nullptr;
    int (*cudaGetDeviceCount)(int*) = nullptr;
    int (*cudaGetDeviceProperties)(cudaDeviceProp*, int) = nullptr;
    int (*cudaRuntimeGetVersion)(int*) = nullptr;
    int (*cudaDriverGetVersion)(int*) = nullptr;

    cuda_runtime_api() {
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
            reinterpret_cast<int (*)(cudaDeviceProp*, int)>(
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

    ~cuda_runtime_api() {
        if (handle) {
            dlclose(handle);
        }
    }
};

struct hip_api {
    void* handle = nullptr;
    int (*hipGetDeviceCount)(int*) = nullptr;
    int (*hipGetDeviceProperties)(hipDeviceProp*, int) = nullptr;
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

        hipGetDeviceProperties = reinterpret_cast<int (*)(hipDeviceProp*, int)>(
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

static bool file_exists(const char* name) {
    struct stat buffer;
    return (stat(name, &buffer) == 0);
}

static bool has_nvidia_gpu() { return file_exists("/dev/nvidiactl"); }

static cudaHipError add_cuda_runtime_devices(std::vector<device>& devices) {
    LOGD("scanning for cuda runtime devices");

    cuda_runtime_api api;

    int dr_ver = 0;
    int rt_ver = 0;
    api.cudaDriverGetVersion(&dr_ver);
    api.cudaRuntimeGetVersion(&rt_ver);

    LOGD("cuda version: driver=" << dr_ver << ", runtime=" << rt_ver);

    int device_count = 0;
    if (auto ret = api.cudaGetDeviceCount(&device_count);
        ret != cudaHipSuccess) {
        LOGW("cudaGetDeviceCount error: " << ret);
        return static_cast<cudaHipError>(ret);
    }

    LOGD("cuda number of devices: " << device_count);

    devices.reserve(devices.size() + device_count);

    for (int i = 0; i < device_count; ++i) {
        cudaDeviceProp props;
        if (auto ret = api.cudaGetDeviceProperties(&props, i);
            ret != cudaHipSuccess)
            continue;
        LOGD("cuda device: " << i << ", name=" << props.name);
        devices.push_back({/*id=*/static_cast<uint32_t>(i), api_t::cuda,
                           /*name=*/props.name,
                           /*platform_name=*/{}});
    }

    return cudaHipSuccess;
}

static void add_cuda_dev_devices(std::vector<device>& devices) {
    LOGD("scanning for cuda devices");

    cuda_dev_api api;

    auto logErr = [&](const char* name, CUresult code) {
        const char* str = nullptr;
        if (auto ret = api.cuGetErrorName(code, &str);
            ret != CUresult::CUDA_SUCCESS) {
            LOGW(name << " error: " << code);
        } else {
            LOGW(name << " error: " << str << " (" << code << ")");
        }
    };

    if (auto ret = api.cuInit(0); ret != CUresult::CUDA_SUCCESS) {
        logErr("cuInit", ret);
        throw std::runtime_error{"cuda init error"};
    }

    int dr_ver = 0;
    if (auto ret = api.cuDriverGetVersion(&dr_ver);
        ret != CUresult::CUDA_SUCCESS) {
        logErr("cuDriverGetVersion", ret);
        throw std::runtime_error{"cuda driver get version error"};
    }

    LOGD("cuda version: driver=" << dr_ver);

    int device_count = 0;
    if (auto ret = api.cuDeviceGetCount(&device_count);
        ret != CUresult::CUDA_SUCCESS) {
        logErr("cuDeviceGetCount", ret);
        throw std::runtime_error{"cuda device get count error"};
    }

    LOGD("cuda number of devices: " << device_count);

    devices.reserve(devices.size() + device_count);

    for (int i = 0; i < device_count; ++i) {
        int dev = 0;
        if (auto ret = api.cuDeviceGet(&dev, i);
            ret != CUresult::CUDA_SUCCESS) {
            LOGW("cuDeviceGet[" << i << "] error: " << ret);
            continue;
        }

        char name[256];
        if (auto ret = api.cuDeviceGetName(name, 256, dev);
            ret != CUresult::CUDA_SUCCESS) {
            LOGW("cuDeviceGetName[" << i << "] error: " << ret);
            continue;
        }

        int comp_cap_major = 0;
        if (auto ret = api.cuDeviceGetAttribute(
                &comp_cap_major,
                CUdevice_attribute::
                    CU_DEVICE_ATTRIBUTE_COMPUTE_CAPABILITY_MAJOR,
                dev);
            ret != CUresult::CUDA_SUCCESS) {
            LOGW("cuDeviceGetAttribute[" << i << "] error: " << ret);
            continue;
        }

        int comp_cap_minor = 0;
        if (auto ret = api.cuDeviceGetAttribute(
                &comp_cap_minor,
                CUdevice_attribute::
                    CU_DEVICE_ATTRIBUTE_COMPUTE_CAPABILITY_MINOR,
                dev);
            ret != CUresult::CUDA_SUCCESS) {
            LOGW("cuDeviceGetAttribute[" << i << "] error: " << ret);
            continue;
        }

        LOGD("cuda device: " << i << ", name=" << name << ", comp-cap="
                             << comp_cap_major << '.' << comp_cap_minor);
        devices.push_back({/*id=*/static_cast<uint32_t>(i), api_t::cuda,
                           /*name=*/name,
                           /*platform_name=*/{}});
    }
}

available_devices_result available_devices(bool cuda, bool hip, bool opencl,
                                           bool opencl_always) {
    available_devices_result result;

    if (hip) add_hip_devices(result.devices);
    if (cuda) result.error = add_cuda_devices(result.devices);
    if (opencl && (opencl_always || result.devices.empty()))
        add_opencl_devices(result.devices);

    return result;
}

error_t add_cuda_devices(std::vector<device>& devices) {
    LOGD("scanning for cuda devices");

    error_t error = error_t::no_error;

    try {
        add_cuda_dev_devices(devices);
    } catch (const std::runtime_error& err) {
        LOGW(err.what());

        if (has_nvidia_gpu()) {
            try {
                if (add_cuda_runtime_devices(devices) == cudaHipUnknownError)
                    error = error_t::cuda_uknown_error;
            } catch (const std::runtime_error& err) {
                LOGW(err.what());
            }
        }
    }

    return error;
}

void add_hip_devices(std::vector<device>& devices) {
    LOGD("scanning for hip devices");

    try {
        hip_api api;

        int dr_ver = 0;
        int rt_ver = 0;
        api.hipDriverGetVersion(&dr_ver);
        api.hipRuntimeGetVersion(&rt_ver);

        LOGD("hip version: driver=" << dr_ver << ", runtime=" << rt_ver);

        int device_count = 0;
        if (auto ret = api.hipGetDeviceCount(&device_count);
            ret != cudaHipSuccess) {
            LOGW("hipGetDeviceCount error: " << ret);
            return;
        }

        LOGD("hip number of devices: " << device_count);

        devices.reserve(devices.size() + device_count);

        for (int i = 0; i < device_count; ++i) {
            hipDeviceProp props;
            if (auto ret = api.hipGetDeviceProperties(&props, i);
                ret != cudaHipSuccess)
                continue;
            LOGD("hip device: " << i << ", name=" << props.name
                                << ", gcn-arch=" << props.gcnArch
                                << ", gcn-arch-name=" << props.gcnArchName);
            devices.push_back({/*id=*/static_cast<uint32_t>(i), api_t::rocm,
                               /*name=*/props.name,
                               /*platform_name=*/props.gcnArchName});
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
            LOGW("clGetPlatformIDs error: " << ret);
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
                LOGW("clGetPlatformInfo for name error: " << ret);
                continue;
            }

            char vendor[max_name_size];
            if (auto ret =
                    api.clGetPlatformInfo(platform_ids[i], CL_PLATFORM_VENDOR,
                                          max_name_size, &vendor, nullptr);
                ret != CL_SUCCESS) {
                LOGW("clGetPlatformInfo for vendor error: " << ret);
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
                LOGW("clGetDeviceIDs error: " << ret);
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
                    LOGW("clGetDeviceInfo for name error: " << ret);
                    continue;
                }

                uint64_t type = 0;
                if (auto ret =
                        api.clGetDeviceInfo(device_ids[j], CL_DEVICE_TYPE,
                                            sizeof(type), &type, nullptr);
                    ret != CL_SUCCESS) {
                    LOGW("clGetDeviceInfo for type error: " << ret);
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
    auto* handle = dlopen(name, RTLD_LAZY);
    if (!handle) {
        LOGW("failed to open " << name << ": " << dlerror());
        return false;
    }

    dlclose(handle);

    return true;
}

bool has_cuda_runtime() { return has_lib("libcudart.so"); }

bool has_cudnn() {
    if (has_lib("libcudnn.so")) return true;
    return has_lib("libcudnn.so.8");
}

void rocm_override_gfx_version(const std::string& arch_version) {
    const auto* value = getenv("HSA_OVERRIDE_GFX_VERSION");
    if (!value) setenv("HSA_OVERRIDE_GFX_VERSION", arch_version.c_str(), 1);
}

std::string rocm_overrided_gfx_version(const std::string& gpu_arch_name) {
    if (gpu_arch_name.find("gfx") != 0 || gpu_arch_name.size() < 4) {
        LOGE("invalid gpu arch name: " << gpu_arch_name);
        return {};
    }

    auto arch_version = std::stoi(gpu_arch_name.substr(3));
    if (arch_version <= 0) {
        LOGE("invalid gpu arch version: " << gpu_arch_name);
        return {};
    }

    auto major = arch_version / 100;
    auto minor = (arch_version - major * 100) / 10;
    auto step = arch_version - major * 100 - minor * 10;

    if (major == 0) {
        major = minor;
        minor = step;
        step = 0;
    }

    /* HSA_OVERRIDE_GFX_VERSION=major.minor.stepping */

    LOGD("rocm overrided gfx version: major=" << major << ", minor=" << minor
                                              << ", stepping=" << step);

    return fmt::format("{}.{}.{}", major, minor, 0);
}

}  // namespace gpu_tools
