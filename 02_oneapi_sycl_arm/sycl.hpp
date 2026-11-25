#ifndef ZYNQ_SYCL_HPP
#define ZYNQ_SYCL_HPP
#include <cstddef>
#include <cstring>
#include <string>
#include <vector>

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
        if (host_ptr_ != nullptr && !data_.empty())
            std::memcpy(host_ptr_, data_.data(), count_ * sizeof(T));
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
private:
    T* ptr_;
    size_t size_;
};

class handler {
public:
    template <typename KernelName = void, typename KernelFunc>
    void parallel_for(range<1> num_work_items, KernelFunc kernel) {
        for (size_t i = 0; i < num_work_items.size(); ++i) kernel(id<1>(i));
    }
};

class device {
public:
    std::string get_info_name() const { return "ARM Cortex-A9 (Zynq-7000 PS - armv7l)"; }
};

class queue {
public:
    device get_device() const { return dev_; }
    template <typename CommandGroupFunc>
    void submit(CommandGroupFunc cgf) { handler cgh; cgf(cgh); }
    void wait() {}
private:
    device dev_;
};
} // namespace sycl
#endif
