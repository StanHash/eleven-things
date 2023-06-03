/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

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
