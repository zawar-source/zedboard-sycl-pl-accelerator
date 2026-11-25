#include <cstdint>
#include <iomanip>
#include <iostream>
#include <vector>
#include "sycl_pl.hpp"

int main() {
    constexpr size_t N = 1024;
    std::vector<int32_t> A(N), B(N), C(N, 0);

    for (size_t i = 0; i < N; ++i) {
        A[i] = static_cast<int32_t>(i + 1);
        B[i] = static_cast<int32_t>((i + 1) * 10);
    }

    sycl::queue q;
    std::cout << "=== Zynq-7000 SYCL-to-PL Hardware Offload Verification ===\n";
    std::cout << "[SYCL Runtime] Target Device : " << q.get_device().get_info_name() << "\n";
    std::cout << "[MMIO Driver ] Interface     : "
              << (q.accelerator().is_physical_mmio()
                      ? "/dev/mem Direct Physical MMIO (0x43C00000)"
                      : "AXI4-Lite Register-Mapped HAL (0x43C00000)")
              << "\n";
    std::cout << "[SYCL Runtime] Offloading parallel_for kernel (N = " << N << ") to PL...\n";

    {
        sycl::buffer<int32_t> bufA(A.data(), sycl::range<1>(N));
        sycl::buffer<int32_t> bufB(B.data(), sycl::range<1>(N));
        sycl::buffer<int32_t> bufC(C.data(), sycl::range<1>(N));

        q.submit([&](sycl::handler& h) {
            sycl::accessor<int32_t> accA(bufA, sycl::access_mode::read);
            sycl::accessor<int32_t> accB(bufB, sycl::access_mode::read);
            sycl::accessor<int32_t> accC(bufC, sycl::access_mode::write);
            h.parallel_for_pl_add<class PLVectorAddKernel>(sycl::range<1>(N), accA, accB, accC);
        });

        q.wait();
    }

    for (size_t i = 0; i < N; ++i) {
        if (C[i] != A[i] + B[i]) {
            std::cerr << "[FAIL] Hardware offload mismatch at index " << i << "\n";
            return 1;
        }
    }

    std::cout << "[AXI4-Lite   ] Register Map  : CTRL=0x43C00000, STATUS=0x43C00004, "
              << "A=0x43C00008, B=0x43C0000C, RES=0x43C00010\n";
    std::cout << "[Telemetry   ] AXI4-Lite Reg Transactions : "
              << q.accelerator().axi_transactions() << " (6 MMIO ops/element)\n";
    std::cout << "[Sample Out  ] C[0] = " << C[0]
              << ", C[15] = " << C[15]
              << ", C[1023] = " << C[1023] << "\n";
    std::cout << "[PASS] SYCL-to-PL Hardware Offload verified across all 1024 elements!\n";
    return 0;
}
