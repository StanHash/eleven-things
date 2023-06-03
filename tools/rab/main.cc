/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include <cstddef>
#include <cstdio>  // FILE, stderr, fprintf
#include <cstdlib> // EXIT_SUCCESS, EXIT_FAILURE
#include <cstring> // memcmp

#include <array>       // std::array
#include <memory>      // std::unique_ptr
#include <string>      // std::string
#include <string_view> // std::string_view
#include <vector>      // std::vector

#include "compression.hh"
#include "helpers.hh"

struct FileDeleter
{
    void operator()(std::FILE * file)
    {
        if (file != nullptr)
            std::fclose(file);
    }
};

int Dump(int argc, char const * argv[])
{
    if (argc != 2)
    {
        std::fprintf(stderr, "ERROR: bad usage (dump requires PACK and DIR parameters).\n");
        return EXIT_FAILURE;
    }

    std::string pack_path { argv[0] };
    std::string dir_path { argv[1] };

    if (dir_path.back() == '/' || dir_path.back() == '\\')
        dir_path.pop_back();

    // helper
    std::unique_ptr<std::FILE, FileDeleter> file_owner { std::fopen(pack_path.c_str(), "rb") };

    std::FILE * file = file_owner.get();

    if (file == nullptr)
    {
        std::fprintf(stderr, "ERROR: couldn't open file '%s'.\n", pack_path.c_str());
        return EXIT_FAILURE;
    }

    // we do the thing here

    std::fseek(file, 0, SEEK_END);
    std::size_t file_size = std::ftell(file);

    if (file_size <= 0x10)
    {
        std::fprintf(stderr, "ERROR: file too small.\n");
        return EXIT_FAILURE;
    }

    std::vector<std::uint8_t> file_data(file_size);

    std::fseek(file, 0, SEEK_SET);

    if (std::fread(file_data.data(), 1, file_data.size(), file) != file_data.size())
    {
        std::fprintf(stderr, "ERROR: head read error.\n");
        return EXIT_FAILURE;
    }

    // close file
    file_owner.reset();

    bool const is_pack = std::memcmp(file_data.data(), "PACK", 4) == 0;
    bool const is_paca = std::memcmp(file_data.data(), "PACA", 4) == 0;

    if (!is_pack && !is_paca)
    {
        std::fprintf(stderr, "WARNING: bad magic (expected PACK or PACA).\n");
        return EXIT_FAILURE;
    }

    if (is_paca)
    {
        file_data = Decompress(file_data);

        if (file_data.size() <= 0x10)
        {
            std::fprintf(stderr, "ERROR: PACA decompression failed.\n");
            return EXIT_FAILURE;
        }

#if 0
        // I used this to debug the decompression code

        std::string output_path(dir_path);
        output_path.push_back('/');
        output_path.append("rab_decompressed_paca.dmp");

        // helper
        std::unique_ptr<std::FILE, FileDeleter> output_file_owner { std::fopen(output_path.c_str(), "wb") };

        std::FILE * output_file = output_file_owner.get();

        if (output_file == nullptr)
        {
            std::fprintf(stderr, "ERROR: couldn't open file for write: '%s'.\n", output_path.c_str());
            return EXIT_FAILURE;
        }

        if (std::fwrite(file_data.data(), 1, file_data.size(), output_file) != file_data.size())
        {
            std::fprintf(stderr, "ERROR: read error when writing to file: '%s'.\n", output_path.c_str());
            return EXIT_FAILURE;
        }
#endif
    }

    std::size_t const declared_file_size = le2h<size_t, 4>(file_data.data() + 0x04);
    std::size_t const unknown_08 = le2h<size_t, 4>(file_data.data() + 0x08);
    std::size_t const entry_count = le2h<std::size_t, 2>(file_data.data() + 0x0C);
    std::size_t const entries_off = le2h<std::size_t, 2>(file_data.data() + 0x0E);

    if (declared_file_size != file_data.size())
    {
        std::fprintf(stderr, "WARNING: declared file size doesn't correspond to real file size.\n");
    }

    std::printf("value of unknown_08: %08zX\n", unknown_08);

    if (entries_off + 4 * entry_count >= file_data.size())
    {
        std::fprintf(stderr, "ERROR: file entry list ends out of file bounds.\n");
        return EXIT_FAILURE;
    }

    for (std::size_t i = 0; i < entry_count; i++)
    {
        std::size_t entry_off_off = entries_off + i * 4;
        std::size_t entry_off = le2h<std::size_t, 4>(file_data.data() + entry_off_off);

        uint8_t const * entry_data = file_data.data() + entry_off;

        // read head head

        if (entry_off + 8 >= file_data.size())
        {
            std::fprintf(stderr, "ERROR: header for file entry %zu ends out of file bounds.\n", i);
            return EXIT_FAILURE;
        }

        std::size_t head_size = le2h<std::size_t, 2>(entry_data + 0x00);
        std::size_t declared_pad = le2h<std::size_t, 2>(entry_data + 0x02);
        std::size_t data_size = le2h<std::size_t, 4>(entry_data + 0x04);

        if (entry_off + head_size >= file_data.size())
        {
            std::fprintf(stderr, "ERROR: file entry %zu ends out of file bounds.\n", i);
            return EXIT_FAILURE;
        }

        // read head name

        char const * beg_str = reinterpret_cast<char const *>(entry_data + 8);
        char const * end_str = reinterpret_cast<char const *>(memchr(beg_str, 0, head_size - 8));

        if (end_str == nullptr)
            end_str = beg_str + head_size - 8;

        std::string head_name(beg_str, end_str);

        if (entry_off + head_size + data_size > file_data.size())
        {
            std::fprintf(stderr, "ERROR: file entry %zu ('%s') ends out of file bounds.\n", i, head_name.c_str());
            return EXIT_FAILURE;
        }

        // print info

        std::size_t next_entry_off = (i + 1 < entry_count)
            ? le2h<std::size_t, 4>(file_data.data() + entries_off + (i + 1) * 4)
            : file_data.size();

        bool is_expected_next_off = next_entry_off == entry_off + head_size + data_size + declared_pad;

        std::printf(
            "file entry %d: %s (%zd bytes, padding: %zX%s)\n", (int)i, head_name.c_str(), data_size, declared_pad,
            is_expected_next_off ? "" : " (declared wrong)");

        // dump data

        std::string output_path(dir_path);
        output_path.push_back('/');
        output_path.append(head_name);

        // helper
        std::unique_ptr<std::FILE, FileDeleter> output_file_owner { std::fopen(output_path.c_str(), "wb") };

        std::FILE * output_file = output_file_owner.get();

        if (output_file == nullptr)
        {
            std::fprintf(stderr, "ERROR: couldn't open file for write: '%s'.\n", output_path.c_str());
            return EXIT_FAILURE;
        }

        if (std::fwrite(entry_data + head_size, 1, data_size, output_file) != data_size)
        {
            std::fprintf(stderr, "ERROR: read error when writing to file: '%s'.\n", output_path.c_str());
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}

int Pack(int argc, char const * argv[])
{
    std::fprintf(stderr, "ERROR: pack is unimplemented.\n");
    return EXIT_FAILURE;
}

int Paca(int argc, char const * argv[])
{
    std::fprintf(stderr, "ERROR: paca is unimplemented.\n");
    return EXIT_FAILURE;
}

int main(int argc, char const * argv[])
{
    if (argc < 2)
    {
        std::fprintf(
            stderr,
            "Usage:\n"
            "  %s dump PACK DIR\n"
            "  %s pack PACK FILES...\n"
            "  %s paca PACA FILES...\n",
            argv[0], argv[0], argv[0]);

        return EXIT_FAILURE;
    }

    std::string_view subcommand { argv[1] };

    if (subcommand == "pack")
    {
        return Pack(argc - 2, argv + 2);
    }
    else if (subcommand == "paca")
    {
        return Paca(argc - 2, argv + 2);
    }
    else if (subcommand == "dump")
    {
        return Dump(argc - 2, argv + 2);
    }
    else
    {
        std::fprintf(stderr, "unknown subcommand: %s.\n", argv[1]);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}