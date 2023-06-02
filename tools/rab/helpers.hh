#ifndef RAB_HELPERS_HH
#define RAB_HELPERS_HH

#include <cinttypes>
#include <cstddef>

template <typename T, std::size_t L = sizeof(T)> T le2h(std::uint8_t const * data)
{
    T result = 0;

    for (std::size_t i = 0; i < L; i++)
    {
        result = result | (((T)data[i]) << (i * 8));
    }

    return result;
}

#endif // RAB_HELPERS_HH
