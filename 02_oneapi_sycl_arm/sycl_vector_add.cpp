#include <iostream>
#include <vector>
#include "sycl.hpp"

int main() {
    constexpr size_t N = 1024;
    std::vector<int> A(N), B(N), C(N, 0);
    for (size_t i = 0; i < N; ++i) {
        A[i] = static_cast<int>(i + 1);
        B[i] = static_cast<int>((i + 1) * 10);
    }

    sycl::queue q;
    std::cout << "[SYCL Runtime] Target Device: " << q.get_device().get_info_name() << "\n";
    std::cout << "[SYCL Runtime] Dispatching parallel_for kernel (N = " << N << ")...\n";

    {
        sycl::buffer<int> bufA(A.data(), sycl::range<1>(N));
        sycl::buffer<int> bufB(B.data(), sycl::range<1>(N));
        sycl::buffer<int> bufC(C.data(), sycl::range<1>(N));

        q.submit([&](sycl::handler& h) {
            sycl::accessor<int> accA(bufA, sycl::access_mode::read);
            sycl::accessor<int> accB(bufB, sycl::access_mode::read);
            sycl::accessor<int> accC(bufC, sycl::access_mode::write);
            h.parallel_for<class VectorAddKernel>(sycl::range<1>(N), [=](sycl::id<1> idx) {
                accC[idx] = accA[idx] + accB[idx];
            });
        });
        q.wait();
    }

    for (size_t i = 0; i < N; ++i) {
        if (C[i] != A[i] + B[i]) {
            std::cerr << "[FAIL] Mismatch at " << i << "\n";
            return 1;
        }
    }

    std::cout << "[Sample Output] C[0] = " << C[0]
              << ", C[15] = " << C[15]
              << ", C[1023] = " << C[1023] << "\n";
    std::cout << "[PASS] SYCL Vector Addition verified on Zynq ARM architecture!\n";
    return 0;
}
