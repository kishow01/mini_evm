#ifndef MINI_EVM_H
#define MINI_EVM_H

#include <stdint.h>
#include <string_view>

namespace mini_evm {
    // forward declaration
    class bytes32;

    using uint256be = bytes32;
    using bytes_view = std::basic_string_view<uint8_t>;

    class bytes32 {
    public:
        uint8_t bytes[32];

        constexpr bytes32(uint8_t _bytes[32] = {}) noexcept : bytes(_bytes) { }

        constexpr explicit bytes32(uint64_t val) noexcept : bytes32({
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            static_cast<uint8_t>(val >> 56),
            static_cast<uint8_t>(val >> 48),
            static_cast<uint8_t>(val >> 40),
            static_cast<uint8_t>(val >> 32),
            static_cast<uint8_t>(val >> 24),
            static_cast<uint8_t>(val >> 16),
            static_cast<uint8_t>(val >> 8),
            static_cast<uint8_t>(val >> 0)
        }) { }

        inline constexpr explicit operator bool() const noexcept;

        inline constexpr operator bytes_view() const noexcept { return {bytes, sizeof(bytes)}; }
    };

    class address {
    public:
        uint8_t bytes[20];

        constexpr address(uint8_t _bytes[20] = {}) noexcept : bytes(_bytes) { }

        constexpr explicit address(uint64_t val) noexcept : bytes({
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            static_cast<uint8_t>(val >> 56),
            static_cast<uint8_t>(val >> 48),
            static_cast<uint8_t>(val >> 40),
            static_cast<uint8_t>(val >> 32),
            static_cast<uint8_t>(val >> 24),
            static_cast<uint8_t>(val >> 16),
            static_cast<uint8_t>(val >> 8),
            static_cast<uint8_t>(val >> 0)
        }) { }

        inline constexpr explicit operator bool() const noexcept;

        inline constexpr operator bytes_view() const noexcept { return {bytes, sizeof(bytes)}; }
    };

    enum class EVM_Stoarge_Status {
        EVM_STORAGE_UNCHARGED = 0,
        EVM_STORAGE_MODIFIED = 1,
        EVM_STORAGE_MODIFIED_AGAIN = 2,
        EVM_STORAGE_ADDED = 3,
        EVM_STORAGE_DELETED = 4
    };

    enum class EVM_Status_Code {
        EVM_SUCCESS = 0,
        EVM_FAILURE = 1,
        EVM_REVERT = 2,
        EVM_OUT_OF_GAS = 3,
        EVM_INVALID_INSTRUCTION = 4,
        EVM_UNDEFINED_INSTRUCTION = 5,
        EVM_STACK_OVERFLOW = 6,
        EVM_STACK_UNDERFLOW = 7,
        EVM_BAD_JUMP_DESTINATION = 8,
        EVM_INVALID_MEMORY_ACCESS = 9,
        EVM_CALL_DEPTH_EXCEEDED = 10,
        EVM_STATIC_MODE_VIOLATION = 11,
        EVM_PRECOMPILE_FAILURE = 12,
        EVM_CONTRACT_VALIDATION_FAILURE = 13,
        EVM_ARGUMENT_OUT_OF_RANGE = 14,
        EVM_WASM_UNREACHABLE_INSTRUCTION = 15,
        EVM_WASM_TRAP = 16,
        EVM_INSUFFICIENT_BALANCE = 17,
        EVM_INTERNAL_ERROR = -1,
        EVM_REJECTED = -2,
        EVM_OUT_OF_MEMORY = -3
    };

    class EVM_Result {
    public:
        EVM_Status_Code status_code;
        int64_t gas_left;
        const uint8_t* output_data;
        size_t output_size;
        address create_address;
        uint8_t padding[4];

        typedef void (*evm_release_result_fn)(const EVM_Result* result);
        evm_release_result_fn release;

        
        EVM_Result(EVM_Status_Code _status_code, 
               int64_t _gas_left, 
               const uint8_t* _output_data, 
               size_t _output_size) noexcept 
            : status_code(_status_code), gas_left(_gas_left), output_data(_output_data), output_size(_output_size) { }

        explicit EVM_Result(const EVM_Result& result) noexcept 
            : status_code(result.status_code), 
              gas_left(result.gas_left), 
              output_data(result.output_data), 
              output_size(result.output_size),
              create_address(result.create_address),
              release(result.release) { }

        ~EVM_Result() noexcept {
            if(release)
                release(this);
        }

        EVM_Result(EVM_Result&& other) noexcept 
            : status_code(result.status_code), 
              gas_left(result.gas_left), 
              output_data(result.output_data), 
              output_size(result.output_size),
              create_address(result.create_address),
              // Disable releasing of the rvalue object
              release(nullptr) { }

        EVM_Result& operator=(EVM_Result&& other) noexcept {
            this->~EVM_Result(); // Release this object
            static_case<EVM_Result&>(*this) = other; // Copy data
            other.release = nullptr; // Disable releasing of the rvalue object
            return *this;
        }

        EVM_Result release_raw() noexcept {
            const EVM_Result out = EVM_Result(this->status_code, this->gas_left, this->output_data, this->output_size);
            out.release = nullptr;
            return out;
        }
    };

    enum class EVM_Call_Kind {
        EVM_CALL = 0,
        EVM_DELEGATECALL = 1,
        EVM_CALLCODE = 2,
        EVM_CREATE = 3,
        EVM_CREATE2 = 4
    };

    struct EVM_Message {
        EVM_Call_Kind kind;
        uint32_t flags;
        int32_t depth;
        int64_t gas;
        address recipient;
        address sender;
        const uint8_t* input_data;
        size_t input_size;
        uint256be value;
        bytes32 create2_salt;
        address code_address;
    };

    struct EVM_Tx_Context {
        uint256be tx_gas_price;
        address tx_origin;
        address block_coinbase;
        int64_t block_number;
        int64_t timestamp;
        int64_t gas_limit;
        uint256be block_prev_randao;
        uint256be chain_id;
        uint256be block_base_fee;
    };

    enum EVM_Access_Status {
        EVM_ACCESS_COLD = 0,
        EVM_ACCESS_WARM = 1
    };

    class HostInterface {
    public:
        virtual ~HostInterface() noexcept = default;
        virtual bool account_exists(const address& addr) const noexcept = 0;
        virtual bytes32 get_storage(const address& addr, const bytes32& key) const noexcept = 0;
        virtual EVM_Stoarge_Status set_storage(const address& addr, 
                                               const bytes32& key, 
                                               const bytes32& value) noexcept = 0;
        virtual uint256be get_balance(const address& addr) const noexcept = 0;
        virtual size_t get_code_size(const address& addr) const noexcept = 0;
        virtual bytes32 get_code_hash(const address& addr) const noexcept = 0;
        virtual size_t copy_code(const address& addr, 
                                 size_t code_offset, 
                                 uint8_t* buffer_data, 
                                 size_t buffer_size) const noexcept = 0;
        virtual void selfdestruct(const address& addr, 
                                  const address& beneficiary) noexcept = 0;
        virtual EVM_Result call(const EVM_Message& msg) noexcept = 0;
        virtual EVM_Tx_Context get_tx_context() const noexcept = 0;
        virtual bytes32 get_block_hash(int64_t block_number) const noexcept = 0;
        virtual void emit_log(const address& addr,
                              const uint8_t* data,
                              size_t data_size,
                              const bytes32 topics[],
                              size_t num_topics) noexcept = 0;
        virtual EVM_Access_Status access_account(const address& addr) noexcept = 0;
        virtual EVM_Access_Status access_storage(const address& addr, const bytes32& key) noexcept = 0;
    };
}



#endif