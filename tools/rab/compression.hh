/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef RAB_COMPRESSION_HH
#define RAB_COMPRESSION_HH

#include <cinttypes> // uint8_t
#include <cstdio>    // FILE

#include <vector> // vector (duh)

// TODO: use C++20 span as input instead I think that would be better

std::vector<std::uint8_t> Decompress(std::vector<std::uint8_t> const & compressed, std::FILE * opt_trace = nullptr);

#endif // RAB_COMPRESSION_HH
