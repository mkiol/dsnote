## Building on Ubuntu 24.04

In order to build on Ubuntu, first install [makedeb](https://www.makedeb.org/).

The full build requires both NVidia CUDA and AMD ROCm.

CUDA should be available in the distro.

For ROCm, follow the official [guide](https://rocm.docs.amd.com/projects/install-on-linux/en/latest/tutorial/quick-start.html) and follow the instructions for Ubuntu 22.04 (jammy).

If you do not to build with support for any of them, simply edit PKGBUILD using any text editor and comment out all the lines in `makedepends_x86_64` and `optdepends_x86_64` with the name of the GPU card you want to disable the support for. For example, to disable support for NVidia, the final result will be:

```
makedepends_x86_64=(
#  'nvidia-cuda-dev'     # Support for GPU acceleration on NVidia GPU
#  'nvidia-cuda-toolkit' # Support for GPU acceleration on NVidia GPU
  'rocm-hip-sdk'        # Support GPU acceleration on AMD GPU
)
optdepends=(
  'ocl-icd-libopencl1'  # Support for GPU acceleration with OpenCL
)
optdepends_x86_64=(
  'intel-opencl-icd'    # Support for Intel GPU acceleration with OpenCL
 # 'libcudart12'         # Support for GPU acceleration on NVidia GPU
 # 'nvidia-cudnn'        # Support for GPU acceleration on NVidia GPU
  'rocblas'             # Support for GPU acceleration on AMD GPU
  'rocm-opencl-runtime' # Support GPU acceleration on AMD GPU
)
```

It is possible to disable the suport for NVidia, AMD or both at the same time.
