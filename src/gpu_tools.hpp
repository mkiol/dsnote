/* Copyright (C) 2023-2024 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef GPU_TOOLS_CPP
#define GPU_TOOLS_CPP

#include <cstdint>
#include <string>
#include <vector>

namespace gpu_tools {
enum class api_t { opencl, cuda, rocm, openvino };
enum class error_t { no_error, cuda_uknown_error };

struct device {
    uint32_t id;
    api_t api = api_t::opencl;
    std::string name;
    std::string platform_name;
};

struct available_devices_result {
    error_t error = error_t::no_error;
    std::vector<device> devices;
};

available_devices_result available_devices(bool cuda, bool hip, bool openvino,
                                           bool opencl, bool opencl_always);
void add_opencl_devices(std::vector<device>& devices);
error_t add_cuda_devices(std::vector<device>& devices);
void add_hip_devices(std::vector<device>& devices);
void add_openvino_devices(std::vector<device>& devices);
bool has_cuda_runtime();
bool has_cudnn();
bool has_openvino();
bool has_nvidia_gpu();
bool has_amd_gpu();
void rocm_override_gfx_version(const std::string& arch_version);
std::string rocm_overrided_gfx_version(const std::string& gpu_arch_name);

}  // namespace gpu_tools

#endif  // GPU_TOOLS_CPP
