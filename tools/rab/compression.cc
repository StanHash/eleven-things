/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "compression.hh"

#include <algorithm> // copy_n

#include "helpers.hh"

// lazy
using namespace std;

vector<uint8_t> Decompress(vector<uint8_t> const & compressed, FILE * opt_trace)
{
    // this is like the NDS backwards LZSS compression used to pack arm9 binaries
    // perhaps exactly the same, except the format of the footer?

    if (compressed.size() < 8)
    {
        if (opt_trace != NULL)
            fprintf(opt_trace, "TRACE: Decompress: payload too small: %zu < 8.\n", compressed.size());

        return {};
    }

    // read footer

    uint8_t const * const footer = compressed.data() + compressed.size() - 8;

    size_t const lower_bound_offset = le2h<size_t, 3>(footer + 0x00);
    size_t const upper_bound_offset = le2h<size_t, 1>(footer + 0x03);
    size_t const unpacked_extend = le2h<size_t, 3>(footer + 0x04);

    if (lower_bound_offset > compressed.size() || upper_bound_offset > lower_bound_offset)
    {
        if (opt_trace != NULL)
        {
            fprintf(
                opt_trace, "TRACE: Decompress: compressed offset (%zX...%zX) out of bounds (0...%zX).\n",
                upper_bound_offset, lower_bound_offset, compressed.size());
        }

        return {};
    }

    uint8_t const * const lower_bound = compressed.data() + compressed.size() - lower_bound_offset;
    uint8_t const * const upper_bound = compressed.data() + compressed.size() - upper_bound_offset;

    vector<uint8_t> result(compressed.size() + unpacked_extend);

    uint8_t const * it = upper_bound;
    uint8_t * ot = result.data() + result.size();

    while (it > lower_bound)
    {
        uint8_t flags = *--it;

        for (size_t i = 0; i < 8; i++)
        {
            if ((flags & 0x80) == 0)
            {
                // store byte verbatim

                if (it - 1 < lower_bound)
                {
                    // this should not happen (should be checked before)
                    return {};
                }

                if (ot - 1 < result.data())
                {
                    // this could happen in case of corrupted input
                    return {};
                }

                *--ot = *--it;
            }
            else
            {
                // lz lookup

                // check error
                if (it - 2 < lower_bound)
                {
                    // this could happen in case of corrupted input
                    return {};
                }

                uint8_t const hi = *--it;
                uint8_t const lo = *--it;

                size_t const len = ((hi & 0xF0) >> 4) + 3;
                size_t const off = (lo | ((hi & 0x0F) << 8)) + 3;

                uint8_t const * ref = ot + off;

                if (ref > result.data() + result.size())
                {
                    // this could happen in case of corrupted input
                    return {};
                }

                if (ot - len < result.data())
                {
                    // this could happen in case of corrupted input
                    return {};
                }

                for (size_t i = 0; i < len; i++)
                {
                    *--ot = *--ref;
                }
            }

            // this is valid end status
            if (it <= lower_bound)
                break;

            flags <<= 1;
        }
    }

    // the size of the head part of the file which is not compressed
    size_t const notcompressed_size = compressed.size() - lower_bound_offset;

    copy_n(compressed.begin(), notcompressed_size, result.begin());

    return result;
}
