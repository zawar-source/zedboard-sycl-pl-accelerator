#ifndef ZYNQ_SYCL_PL_HPP
#define ZYNQ_SYCL_PL_HPP

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include "zynq_pl_hal.hpp"

namespace sycl {

template <int Dimensions = 1>
class range {
public:
    explicit range(size_t dim0) : dim0_(dim0) {}
    size_t size() const { return dim0_; }
private:
    size_t dim0_;
};

template <int Dimensions = 1>
class id {
public:
    explicit id(size_t idx0) : idx0_(idx0) {}
    operator size_t() const { return idx0_; }
private:
    size_t idx0_;
};

enum class access_mode { read, write, read_write };

template <typename T>
class buffer {
public:
    buffer(T* host_data, range<1> r)
        : host_ptr_(host_data), count_(r.size()), data_(host_data, host_data + r.size()) {}
    ~buffer() {
        if (host_ptr_ != nullptr && !data_.empty()) {
            std::memcpy(host_ptr_, data_.data(), count_ * sizeof(T));
        }
    }
    T* data() { return data_.data(); }
    size_t size() const { return count_; }
private:
    T* host_ptr_;
    size_t count_;
    std::vector<T> data_;
};

template <typename T>
class accessor {
public:
    accessor(buffer<T>& buf, access_mode = access_mode::read_write)
        : ptr_(buf.data()), size_(buf.size()) {}
    T& operator[](id<1> idx) const { return ptr_[static_cast<size_t>(idx)]; }
    T& operator[](size_t i) const { return ptr_[i]; }
    size_t size() const { return size_; }
private:
    T* ptr_;
    size_t size_;
};

class device {
public:
    std::string get_info_name() const {
        return "Zynq-7000 PL Custom AXI4-Lite Accelerator (Base: 0x43C00000)";
    }
};

class handler {
public:
    explicit handler(ZynqPLAccelerator& pl) : pl_(pl) {}

    template <typename KernelName = void>
    void parallel_for_pl_add(range<1> num_items,
                             accessor<int32_t>& accA,
                             accessor<int32_t>& accB,
                             accessor<int32_t>& accC) {
        for (size_t i = 0; i < num_items.size(); ++i) {
            accC[i] = pl_.offload_vector_add(accA[i], accB[i]);
        }
    }

private:
    ZynqPLAccelerator& pl_;
};

class queue {
public:
    device get_device() const { return dev_; }
    const ZynqPLAccelerator& accelerator() const { return pl_; }

    template <typename CommandGroupFunc>
    void submit(CommandGroupFunc cgf) {
        handler cgh(pl_);
        cgf(cgh);
    }

    void wait() {}

private:
    device dev_;
    ZynqPLAccelerator pl_;
};

} // namespace sycl

#endif // ZYNQ_SYCL_PL_HPP
