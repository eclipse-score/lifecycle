#ifndef SCORE_HM_FFI_HELPERS_HPP
#define SCORE_HM_FFI_HELPERS_HPP

namespace score::hm::ffi
{

inline Error fromRustError(int ffi_error_code)
{
    switch (ffi_error_code)
    {
        case 1:
            return Error::NotFound;
        case 2:
            return Error::AlreadyExists;
        case 3:
            return Error::InvalidArgument;
        case 4:
            return Error::WrongState;
        case 5:
            return Error::Failed;
        default:
            assert(false && "Unknown FFI error code");
            return Error::InvalidArgument;  // Fallback
    }
}

}  // namespace score::hm::ffi

#endif  // SCORE_HM_FFI_HELPERS_HPP
