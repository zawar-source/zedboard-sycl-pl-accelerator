#ifndef ZYNQ_PL_HAL_HPP
#define ZYNQ_PL_HAL_HPP

#include <cstdint>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

// Must match Vivado Address Editor base address & pl_vec_add_axi.v offsets
constexpr uint32_t PL_ACCEL_BASE_ADDR = 0x43C00000;
constexpr size_t   PL_ACCEL_MAP_SIZE  = 0x1000;

constexpr uint32_t REG_CTRL      = 0x00; // Write 1 = Start, 0 = Reset done
constexpr uint32_t REG_STATUS    = 0x04; // Read bit[0] = done_flag
constexpr uint32_t REG_OPERAND_A = 0x08; // Write 32-bit signed A[i]
constexpr uint32_t REG_OPERAND_B = 0x0C; // Write 32-bit signed B[i]
constexpr uint32_t REG_RESULT    = 0x10; // Read 32-bit signed A[i] + B[i]

class ZynqPLAccelerator {
public:
    ZynqPLAccelerator()
        : fd_(-1), mapped_base_(nullptr), hw_mapped_(false),
          reg_ctrl_(0), reg_status_(0), reg_a_(0), reg_b_(0), reg_res_(0),
          axi_tx_count_(0) {
        fd_ = open("/dev/mem", O_RDWR | O_SYNC);
        if (fd_ >= 0) {
            void* ptr = mmap(nullptr, PL_ACCEL_MAP_SIZE, PROT_READ | PROT_WRITE,
                             MAP_SHARED, fd_, PL_ACCEL_BASE_ADDR);
            if (ptr != MAP_FAILED) {
                mapped_base_ = ptr;
                hw_mapped_ = true;
            }
        }
    }

    ~ZynqPLAccelerator() {
        if (hw_mapped_ && mapped_base_ != nullptr) {
            munmap(mapped_base_, PL_ACCEL_MAP_SIZE);
        }
        if (fd_ >= 0) {
            close(fd_);
        }
    }

    bool is_physical_mmio() const { return hw_mapped_; }
    uint64_t axi_transactions() const { return axi_tx_count_; }

    void write32(uint32_t offset, uint32_t val) {
        ++axi_tx_count_;
        if (hw_mapped_) {
            volatile uint32_t* reg = reinterpret_cast<volatile uint32_t*>(
                static_cast<uint8_t*>(mapped_base_) + offset);
            *reg = val;
            return;
        }
        // Exact RTL register behavior from pl_vec_add_axi.v for QEMU user-mode
        switch (offset) {
            case REG_CTRL:
                reg_ctrl_ = val;
                if ((reg_ctrl_ & 0x1U) == 0U) {
                    reg_status_ = 0U;
                } else if ((reg_status_ & 0x1U) == 0U) {
                    reg_res_ = static_cast<uint32_t>(
                        static_cast<int32_t>(reg_a_) + static_cast<int32_t>(reg_b_));
                    reg_status_ = 1U;
                }
                break;
            case REG_OPERAND_A: reg_a_ = val; break;
            case REG_OPERAND_B: reg_b_ = val; break;
            default: break;
        }
    }

    uint32_t read32(uint32_t offset) {
        ++axi_tx_count_;
        if (hw_mapped_) {
            volatile uint32_t* reg = reinterpret_cast<volatile uint32_t*>(
                static_cast<uint8_t*>(mapped_base_) + offset);
            return *reg;
        }
        switch (offset) {
            case REG_CTRL:      return reg_ctrl_;
            case REG_STATUS:    return reg_status_;
            case REG_OPERAND_A: return reg_a_;
            case REG_OPERAND_B: return reg_b_;
            case REG_RESULT:    return reg_res_;
            default:            return 0U;
        }
    }

    int32_t offload_vector_add(int32_t a, int32_t b) {
        write32(REG_OPERAND_A, static_cast<uint32_t>(a));
        write32(REG_OPERAND_B, static_cast<uint32_t>(b));
        write32(REG_CTRL, 1U);
        while ((read32(REG_STATUS) & 0x1U) == 0U) {}
        int32_t res = static_cast<int32_t>(read32(REG_RESULT));
        write32(REG_CTRL, 0U);
        return res;
    }

private:
    int fd_;
    void* mapped_base_;
    bool hw_mapped_;
    uint32_t reg_ctrl_, reg_status_, reg_a_, reg_b_, reg_res_;
    uint64_t axi_tx_count_;
};

#endif // ZYNQ_PL_HAL_HPP
