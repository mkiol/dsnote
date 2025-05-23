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
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
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

enum ov_status_e {
    OK = 0,  //!< SUCCESS
    /*
     * @brief map exception to C++ interface
     */
    GENERAL_ERROR = -1,       //!< GENERAL_ERROR
    NOT_IMPLEMENTED = -2,     //!< NOT_IMPLEMENTED
    NETWORK_NOT_LOADED = -3,  //!< NETWORK_NOT_LOADED
    PARAMETER_MISMATCH = -4,  //!< PARAMETER_MISMATCH
    NOT_FOUND = -5,           //!< NOT_FOUND
    OUT_OF_BOUNDS = -6,       //!< OUT_OF_BOUNDS
    /*
     * @brief exception not of std::exception derived type was thrown
     */
    UNEXPECTED = -7,          //!< UNEXPECTED
    REQUEST_BUSY = -8,        //!< REQUEST_BUSY
    RESULT_NOT_READY = -9,    //!< RESULT_NOT_READY
    NOT_ALLOCATED = -10,      //!< NOT_ALLOCATED
    INFER_NOT_STARTED = -11,  //!< INFER_NOT_STARTED
    NETWORK_NOT_READ = -12,   //!< NETWORK_NOT_READ
    INFER_CANCELLED = -13,    //!< INFER_CANCELLED
    /*
     * @brief exception in C wrapper
     */
    INVALID_C_PARAM = -14,         //!< INVALID_C_PARAM
    UNKNOWN_C_ERROR = -15,         //!< UNKNOWN_C_ERROR
    NOT_IMPLEMENT_C_METHOD = -16,  //!< NOT_IMPLEMENT_C_METHOD
    UNKNOW_EXCEPTION = -17,        //!< UNKNOW_EXCEPTION
};

struct ov_version {
    const char* buildNumber;
    const char* description;
};

struct ov_available_devices {
    char** devices;
    size_t size;
};

#define VK_MAKE_API_VERSION(variant, major, minor, patch)            \
    ((((uint32_t)(variant)) << 29U) | (((uint32_t)(major)) << 22U) | \
     (((uint32_t)(minor)) << 12U) | ((uint32_t)(patch)))
#define VK_API_VERSION_1_0 VK_MAKE_API_VERSION(0, 1, 0, 0)
#define VK_MAKE_VERSION(major, minor, patch)                       \
    ((((uint32_t)(major)) << 22U) | (((uint32_t)(minor)) << 12U) | \
     ((uint32_t)(patch)))

enum VkResult {
    VK_SUCCESS = 0,
    VK_NOT_READY = 1,
    VK_TIMEOUT = 2,
    VK_EVENT_SET = 3,
    VK_EVENT_RESET = 4,
    VK_INCOMPLETE = 5,
    VK_ERROR_OUT_OF_HOST_MEMORY = -1,
    VK_ERROR_OUT_OF_DEVICE_MEMORY = -2,
    VK_ERROR_INITIALIZATION_FAILED = -3,
    VK_ERROR_DEVICE_LOST = -4,
    VK_ERROR_MEMORY_MAP_FAILED = -5,
    VK_ERROR_LAYER_NOT_PRESENT = -6,
    VK_ERROR_EXTENSION_NOT_PRESENT = -7,
    VK_ERROR_FEATURE_NOT_PRESENT = -8,
    VK_ERROR_INCOMPATIBLE_DRIVER = -9,
    VK_ERROR_TOO_MANY_OBJECTS = -10,
    VK_ERROR_FORMAT_NOT_SUPPORTED = -11,
    VK_ERROR_FRAGMENTED_POOL = -12,
    VK_ERROR_UNKNOWN = -13
};

enum VkStructureType {
    VK_STRUCTURE_TYPE_APPLICATION_INFO = 0,
    VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO = 1
};

enum VkInstanceCreateFlags {
    VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR = 0x00000001
};

enum VkPhysicalDeviceType {
    VK_PHYSICAL_DEVICE_TYPE_OTHER = 0,
    VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU = 1,
    VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU = 2,
    VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU = 3,
    VK_PHYSICAL_DEVICE_TYPE_CPU = 4
};

#define VK_FALSE 0U
#define VK_LOD_CLAMP_NONE 1000.0F
#define VK_QUEUE_FAMILY_IGNORED (~0U)
#define VK_REMAINING_ARRAY_LAYERS (~0U)
#define VK_REMAINING_MIP_LEVELS (~0U)
#define VK_SUBPASS_EXTERNAL (~0U)
#define VK_TRUE 1U
#define VK_WHOLE_SIZE (~0ULL)
#define VK_MAX_MEMORY_TYPES 32U
#define VK_MAX_PHYSICAL_DEVICE_NAME_SIZE 256U
#define VK_UUID_SIZE 16U
#define VK_MAX_EXTENSION_NAME_SIZE 256U
#define VK_MAX_DESCRIPTION_SIZE 256U
#define VK_MAX_MEMORY_HEAPS 16U

struct VkExtensionProperties {
    char extensionName[VK_MAX_EXTENSION_NAME_SIZE];
    uint32_t specVersion;
};

struct VkApplicationInfo {
    VkStructureType sType;
    const void* pNext;
    const char* pApplicationName;
    uint32_t applicationVersion;
    const char* pEngineName;
    uint32_t engineVersion;
    uint32_t apiVersion;
};

struct VkInstanceCreateInfo {
    VkStructureType sType;
    const void* pNext;
    VkInstanceCreateFlags flags;
    const VkApplicationInfo* pApplicationInfo;
    uint32_t enabledLayerCount;
    const char* const* ppEnabledLayerNames;
    uint32_t enabledExtensionCount;
    const char* const* ppEnabledExtensionNames;
};

struct VkPhysicalDeviceProperties {
    uint32_t apiVersion;
    uint32_t driverVersion;
    uint32_t vendorID;
    uint32_t deviceID;
    VkPhysicalDeviceType deviceType;
    char deviceName[VK_MAX_PHYSICAL_DEVICE_NAME_SIZE];
    char padding[1024];
};

struct VkPhysicalDeviceProperties2 {
    VkStructureType sType;
    void* pNext;
    VkPhysicalDeviceProperties properties;
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
    CUresult (*cuInit)(unsigned int Flags) = nullptr;
    CUresult (*cuGetErrorName)(CUresult error, const char** pStr) = nullptr;
    CUresult (*cuDriverGetVersion)(int* driverVersion) = nullptr;
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

struct openvino_api {
    void* handle = nullptr;
    ov_status_e (*ov_get_openvino_version)(ov_version* version) = nullptr;
    void (*ov_version_free)(ov_version* version) = nullptr;
    ov_status_e (*ov_core_create)(void** core) = nullptr;
    void (*ov_core_free)(void* core) = nullptr;
    ov_status_e (*ov_core_get_available_devices)(
        const void* core, ov_available_devices* devices) = nullptr;
    void (*ov_available_devices_free)(ov_available_devices* devices) = nullptr;
    ov_status_e (*ov_core_get_property)(const void* core,
                                        const char* device_name,
                                        const char* property_key,
                                        char** property_value) = nullptr;

    openvino_api() {
        handle = dlopen("libopenvino_c.so", RTLD_LAZY);
        if (!handle) {
            LOGW("failed to open openvino lib: " << dlerror());
            throw std::runtime_error("failed to open openvino lib");
        }

        ov_get_openvino_version =
            reinterpret_cast<decltype(ov_get_openvino_version)>(
                dlsym(handle, "ov_get_openvino_version"));
        if (!ov_get_openvino_version) {
            LOGW("failed to sym ov_get_openvino_version");
            dlclose(handle);
            throw std::runtime_error("failed to sym ov_get_openvino_version");
        }

        ov_version_free = reinterpret_cast<decltype(ov_version_free)>(
            dlsym(handle, "ov_version_free"));
        if (!ov_version_free) {
            LOGW("failed to sym ov_version_free");
            dlclose(handle);
            throw std::runtime_error("failed to sym ov_version_free");
        }

        ov_core_create = reinterpret_cast<decltype(ov_core_create)>(
            dlsym(handle, "ov_core_create"));
        if (!ov_core_create) {
            LOGW("failed to sym ov_core_create");
            dlclose(handle);
            throw std::runtime_error("failed to sym ov_core_create");
        }

        ov_core_free = reinterpret_cast<decltype(ov_core_free)>(
            dlsym(handle, "ov_core_free"));
        if (!ov_core_free) {
            LOGW("failed to sym ov_core_free");
            dlclose(handle);
            throw std::runtime_error("failed to sym ov_core_free");
        }

        ov_core_get_available_devices =
            reinterpret_cast<decltype(ov_core_get_available_devices)>(
                dlsym(handle, "ov_core_get_available_devices"));
        if (!ov_core_get_available_devices) {
            LOGW("failed to sym ov_core_get_available_devices");
            dlclose(handle);
            throw std::runtime_error(
                "failed to sym ov_core_get_available_devices");
        }

        ov_available_devices_free =
            reinterpret_cast<decltype(ov_available_devices_free)>(
                dlsym(handle, "ov_available_devices_free"));
        if (!ov_available_devices_free) {
            LOGW("failed to sym ov_available_devices_free");
            dlclose(handle);
            throw std::runtime_error("failed to sym ov_available_devices_free");
        }

        ov_core_get_property = reinterpret_cast<decltype(ov_core_get_property)>(
            dlsym(handle, "ov_core_get_property"));
        if (!ov_core_get_property) {
            LOGW("failed to sym ov_core_get_property");
            dlclose(handle);
            throw std::runtime_error("failed to sym ov_core_get_property");
        }
    }

    ~openvino_api() {
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

struct vulkan_api {
    void* handle = nullptr;
    VkResult (*vkEnumerateInstanceExtensionProperties)(
        const char*, uint32_t*, VkExtensionProperties*) = nullptr;
    VkResult (*vkCreateInstance)(const VkInstanceCreateInfo*, const void*,
                                 void**) = nullptr;
    VkResult (*vkEnumeratePhysicalDevices)(void*, uint32_t*, void**) = nullptr;
    void (*vkGetPhysicalDeviceProperties2)(
        void*, VkPhysicalDeviceProperties2*) = nullptr;
    void (*vkDestroyInstance)(void*, const void*) = nullptr;

    vulkan_api() {
        handle = dlopen("libvulkan.so.1", RTLD_LAZY);
        if (!handle) {
            LOGW("failed to open vulkan lib: " << dlerror());
            throw std::runtime_error("failed to open vulkan lib");
        }

        vkEnumerateInstanceExtensionProperties =
            reinterpret_cast<decltype(vkEnumerateInstanceExtensionProperties)>(
                dlsym(handle, "vkEnumerateInstanceExtensionProperties"));
        if (!vkEnumerateInstanceExtensionProperties) {
            LOGW("failed to sym vkEnumerateInstanceExtensionProperties");
            dlclose(handle);
            throw std::runtime_error(
                "failed to sym vkEnumerateInstanceExtensionProperties");
        }

        vkCreateInstance = reinterpret_cast<decltype(vkCreateInstance)>(
            dlsym(handle, "vkCreateInstance"));
        if (!vkCreateInstance) {
            LOGW("failed to sym vkCreateInstance");
            dlclose(handle);
            throw std::runtime_error("failed to sym vkCreateInstance");
        }

        vkEnumeratePhysicalDevices =
            reinterpret_cast<decltype(vkEnumeratePhysicalDevices)>(
                dlsym(handle, "vkEnumeratePhysicalDevices"));
        if (!vkEnumeratePhysicalDevices) {
            LOGW("failed to sym vkEnumeratePhysicalDevices");
            dlclose(handle);
            throw std::runtime_error(
                "failed to sym vkEnumeratePhysicalDevices");
        }

        vkGetPhysicalDeviceProperties2 =
            reinterpret_cast<decltype(vkGetPhysicalDeviceProperties2)>(
                dlsym(handle, "vkGetPhysicalDeviceProperties2"));
        if (!vkGetPhysicalDeviceProperties2) {
            LOGW("failed to sym vkGetPhysicalDeviceProperties2");
            dlclose(handle);
            throw std::runtime_error(
                "failed to sym vkGetPhysicalDeviceProperties2");
        }

        vkDestroyInstance = reinterpret_cast<decltype(vkDestroyInstance)>(
            dlsym(handle, "vkDestroyInstance"));
        if (!vkDestroyInstance) {
            LOGW("failed to sym vkDestroyInstance");
            dlclose(handle);
            throw std::runtime_error("failed to sym vkDestroyInstance");
        }
    }

    ~vulkan_api() {
        if (handle) {
            dlclose(handle);
        }
    }
};

[[maybe_unused]] static bool file_exists(const char* name) {
    struct stat buffer {};
    return (stat(name, &buffer) == 0);
}

bool has_nvidia_gpu() {
#ifdef ARCH_X86_64
    return file_exists("/dev/nvidiactl");
#else
    return false;
#endif
}

bool has_amd_gpu() {
#ifdef ARCH_X86_64
    return file_exists("/dev/kfd");
#else
    return false;
#endif
}

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

void add_opencl_devices(std::vector<device>& devices, uint8_t flags) {
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

                if ((type & CL_DEVICE_TYPE_GPU) == 0) {
                    LOGD("opencl unsupported device => skipping");
                    continue;
                }

                bool is_clover = strcmp("Clover", pname) == 0;

                if (is_clover && (flags & scan_flags_t::opencl_clover) == 0) {
                    LOGD("opencl clover device => skipping");
                    continue;
                }

                if (!is_clover && (flags & scan_flags_t::opencl_default) == 0) {
                    LOGD("opencl default device => skipping");
                    continue;
                }

                devices_in_platform.second.push_back({/*id=*/i, api_t::opencl,
                                                      /*name=*/dname,
                                                      /*platform_name=*/pname});
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

void add_openvino_devices(std::vector<device>& devices, uint8_t flags) {
    LOGD("scanning for openvino devices");

    try {
        openvino_api api;

        ov_version version{};
        if (auto ret = api.ov_get_openvino_version(&version);
            ret != ov_status_e::OK) {
            LOGE("ov_get_openvino_version error: " << ret);
            return;
        }

        LOGD("openvino version: build="
             << version.buildNumber << ", description=" << version.description);

        api.ov_version_free(&version);

        void* core = nullptr;

        if (auto ret = api.ov_core_create(&core); ret != ov_status_e::OK) {
            LOGE("ov_core_create error: " << ret);
            return;
        }

        if (!core) {
            LOGE("ov core is nil");
            return;
        }

        ov_available_devices available_devices{};

        if (auto ret =
                api.ov_core_get_available_devices(core, &available_devices);
            ret != ov_status_e::OK) {
            LOGE("ov_core_get_available_devices error: " << ret);
            api.ov_core_free(core);
            return;
        }

        LOGD("openvino number of devices: " << available_devices.size);

        for (size_t i = 0; i < available_devices.size; ++i) {
            char* device_full_name = nullptr;
            api.ov_core_get_property(core, available_devices.devices[i],
                                     "FULL_DEVICE_NAME", &device_full_name);
            LOGD("openvino device: "
                 << i << ", name=" << available_devices.devices[i]
                 << ", full-name="
                 << (device_full_name ? device_full_name : "Unknown"));

            bool is_gpu = std::string_view{available_devices.devices[i]}.find(
                              "GPU") != std::string_view::npos;

            if (is_gpu && (flags & scan_flags_t::openvino_gpu) == 0) {
                LOGD("openvino gpu device => skipping");
                continue;
            }

            if (!is_gpu && (flags & scan_flags_t::openvino_default) == 0) {
                LOGD("openvino default device => skipping");
                continue;
            }

            devices.push_back({/*id=*/static_cast<uint32_t>(i), api_t::openvino,
                               /*name=*/available_devices.devices[i],
                               /*platform_name=*/device_full_name});
        }

        api.ov_available_devices_free(&available_devices);
        api.ov_core_free(core);
    } catch ([[maybe_unused]] const std::runtime_error& err) {
    }
}

void add_vulkan_devices(std::vector<device>& devices, uint8_t flags) {
    LOGD("scanning for vulkan devices");

    try {
        vulkan_api api;

        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "dsnote";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "dsnote";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        uint32_t extensionsCount = 0;
        if (auto ret = api.vkEnumerateInstanceExtensionProperties(
                nullptr, &extensionsCount, nullptr);
            ret != VK_SUCCESS) {
            LOGE("vkEnumerateInstanceExtensionProperties error: " << ret);
            return;
        }

        if (extensionsCount > 0) {
            std::vector<VkExtensionProperties> extensions(extensionsCount);
            if (auto ret = api.vkEnumerateInstanceExtensionProperties(
                    nullptr, &extensionsCount, extensions.data());
                ret != VK_SUCCESS) {
                LOGE("vkEnumerateInstanceExtensionProperties error: " << ret);
                return;
            }

            std::ostringstream ss;
            for (const auto& ext : extensions) ss << ext.extensionName << ",";
            LOGD("vulkan extensions: " << ss.str());
        }

        void* instance = nullptr;
        if (auto ret = api.vkCreateInstance(&createInfo, nullptr, &instance);
            ret != VK_SUCCESS) {
            LOGE("vkCreateInstance error: " << ret);
            return;
        }

        uint32_t pdevicesCount = 0;
        if (auto ret = api.vkEnumeratePhysicalDevices(instance, &pdevicesCount,
                                                      nullptr);
            ret != VK_SUCCESS) {
            LOGE("vkEnumeratePhysicalDevices error: " << ret);

            api.vkDestroyInstance(instance, nullptr);
            return;
        }

        if (pdevicesCount == 0) {
            api.vkDestroyInstance(instance, nullptr);
            return;
        }

        std::vector<void*> pdevices(pdevicesCount);
        if (auto ret = api.vkEnumeratePhysicalDevices(instance, &pdevicesCount,
                                                      pdevices.data());
            ret != VK_SUCCESS) {
            LOGE("vkEnumeratePhysicalDevices error: " << ret);
            api.vkDestroyInstance(instance, nullptr);
            return;
        }

        auto type_str = [](VkPhysicalDeviceType type) {
            switch (type) {
                case VK_PHYSICAL_DEVICE_TYPE_OTHER:
                    return "other";
                case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                    return "integrated-gpu";
                case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                    return "discrete-gpu";
                case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                    return "virtual-gpu";
                case VK_PHYSICAL_DEVICE_TYPE_CPU:
                    return "cpu";
                default:
                    return "unknown";
            }
        };

        const size_t max_nb_of_devices =
            16;  // whisper.cpp supports up to 16 vulkan devs

        for (size_t i = 0; i < pdevices.size(); ++i) {
            const auto& device = pdevices[i];

            VkPhysicalDeviceProperties2 prop{};
            api.vkGetPhysicalDeviceProperties2(device, &prop);

            LOGD("vulkan device: " << i << " id=" << prop.properties.deviceID
                                   << ", name=" << prop.properties.deviceName
                                   << ", type="
                                   << type_str(prop.properties.deviceType));

            if (prop.properties.deviceType !=
                    VkPhysicalDeviceType::
                        VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
                prop.properties.deviceType !=
                    VkPhysicalDeviceType::
                        VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU &&
                prop.properties.deviceType !=
                    VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_CPU) {
                LOGD("vulkan unsupported device => skipping");
                continue;
            }

            bool is_igpu =
                prop.properties.deviceType ==
                VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU;

            if (is_igpu && (flags & scan_flags_t::vulkan_igpu) == 0) {
                LOGD("vulkan igpu device => skipping");
                continue;
            }

            bool is_cpu = prop.properties.deviceType ==
                          VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_CPU;

            if (is_cpu && (flags & scan_flags_t::vulkan_cpu) == 0) {
                LOGD("vulkan cpu device => skipping");
                continue;
            }

            if (!is_igpu && !is_cpu &&
                (flags & scan_flags_t::vulkan_default) == 0) {
                LOGD("vulkan default device => skipping");
                continue;
            }

            if (i >= max_nb_of_devices) {
                LOGD("already have the max nb of vulkan devices => skipping");
                continue;
            }

            devices.push_back({/*id=*/static_cast<uint32_t>(i), api_t::vulkan,
                               /*name=*/prop.properties.deviceName,
                               /*platform_name=*/{}});
        }

        api.vkDestroyInstance(instance, nullptr);
    } catch ([[maybe_unused]] const std::runtime_error& err) {
    }
}

available_devices_result available_devices(
    [[maybe_unused]] bool cuda, [[maybe_unused]] bool hip,
    [[maybe_unused]] bool vulkan, [[maybe_unused]] bool vulkan_igpu,
    [[maybe_unused]] bool vulkan_cpu, [[maybe_unused]] bool openvino,
    [[maybe_unused]] bool openvino_gpu, [[maybe_unused]] bool opencl,
    [[maybe_unused]] bool opencl_clover) {
    available_devices_result result;

#ifdef ARCH_X86_64
    if (cuda) {
        result.error = add_cuda_devices(result.devices);
    }
#endif
#ifndef ARCH_ARM_32
    if (vulkan || vulkan_igpu || vulkan_cpu) {
        uint8_t flags = scan_flags_t::none;
        if (vulkan) flags |= scan_flags_t::vulkan_default;
        if (vulkan_igpu) flags |= scan_flags_t::vulkan_igpu;
        if (vulkan_cpu) flags |= scan_flags_t::vulkan_cpu;
        add_vulkan_devices(result.devices, flags);
    }
#endif
#ifdef ARCH_X86_64
    if (hip) {
        add_hip_devices(result.devices);
    }

    if (openvino || openvino_gpu) {
        uint8_t flags = scan_flags_t::none;
        if (openvino) flags |= scan_flags_t::openvino_default;
        if (openvino_gpu) flags |= scan_flags_t::openvino_gpu;
        add_openvino_devices(result.devices, flags);
    }

    if (opencl || opencl_clover) {
        uint8_t flags = scan_flags_t::none;
        if (opencl) flags |= scan_flags_t::opencl_default;
        if (opencl_clover) flags |= scan_flags_t::opencl_clover;
        add_opencl_devices(result.devices, flags);
    }
#endif
    return result;
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

bool has_cuda_runtime() {
#ifdef ARCH_X86_64
    return has_lib("libcudart.so");
#else
    return false;
#endif
}

bool has_cudnn() {
#ifdef ARCH_X86_64
    return has_lib("libcudnn.so") || has_lib("libcudnn.so.9") ||
           has_lib("libcudnn.so.8");
#else
    return false;
#endif
}

bool has_hip() {
#ifdef ARCH_X86_64
    return has_lib("libamdhip64.so");
#else
    return false;
#endif
}

bool has_openvino() {
#ifdef ARCH_X86_64
    return has_lib("libopenvino_c.so");
#else
    return false;
#endif
}

bool has_vulkan() {
#ifdef ARCH_ARM_32
    return false;
#else
    return has_lib("libvulkan.so.1");
#endif
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
