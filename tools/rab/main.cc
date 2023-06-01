#include <cstddef>
#include <cstdio>  // FILE, stderr, fprintf
#include <cstdlib> // EXIT_SUCCESS, EXIT_FAILURE
#include <cstring> // memcmp

#include <array>       // std::array
#include <memory>      // std::unique_ptr
#include <string>      // std::string
#include <string_view> // std::string_view
#include <vector>      // std::vector

struct FileDeleter
{
    void operator()(std::FILE * file)
    {
        if (file != nullptr)
            std::fclose(file);
    }
};

template <typename T, std::size_t L = sizeof(T)> T le2h(std::uint8_t const * data)
{
    T result = 0;

    for (std::size_t i = 0; i < L; i++)
    {
        result = result | (((T)data[i]) << (i * 8));
    }

    return result;
}

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

    std::fseek(file, 0, SEEK_SET);

    std::array<std::uint8_t, 0x10> head_data;

    if (std::fread(head_data.data(), 1, head_data.size(), file) != head_data.size())
    {
        std::fprintf(stderr, "ERROR: head read error.\n");
        return EXIT_FAILURE;
    }

    if (std::memcmp(head_data.data(), "PACK", 4) != 0)
    {
        if (std::memcmp(head_data.data(), "PACA", 4) != 0)
        {
            std::fprintf(stderr, "WARNING: bad magic (expected PACK or PACA).\n");
        }
        else
        {
            std::fprintf(stderr, "WARNING: we don't really know how to dump PACA files yet.\n");
        }
    }

    std::size_t entry_count = le2h<std::size_t, 2>(head_data.data() + 0x0C);
    std::size_t entries_off = le2h<std::size_t, 2>(head_data.data() + 0x0E);

    std::array<std::uint8_t, 4> entry_buf;

    for (std::size_t i = 0; i < entry_count; i++)
    {
        std::fseek(file, entries_off + i * 4, SEEK_SET);

        if (std::fread(entry_buf.data(), 1, entry_buf.size(), file) != entry_buf.size())
        {
            std::fprintf(stderr, "ERROR: entry offset read error.\n");
            return EXIT_FAILURE;
        }

        std::size_t entry_off = le2h<std::size_t, 4>(entry_buf.data());

        std::vector<std::uint8_t> entry_data;

        std::fseek(file, entry_off, SEEK_SET);

        // we read thrice: one with size 8, one with size head_size - 8, and one final one with data_size

        // read head head

        entry_data.resize(8);

        if (std::fread(entry_data.data(), 1, entry_data.size(), file) != entry_data.size())
        {
            std::fprintf(stderr, "ERROR: entry head head read error.\n");
            return EXIT_FAILURE;
        }

        std::size_t head_size = le2h<std::size_t, 2>(entry_data.data() + 0x00);
        std::size_t unknown_2 = le2h<std::size_t, 2>(entry_data.data() + 0x02);
        std::size_t data_size = le2h<std::size_t, 4>(entry_data.data() + 0x04);

        entry_data.resize(head_size + data_size);

        // read head name

        if (std::fread(entry_data.data() + 8, 1, head_size - 8, file) != head_size - 8)
        {
            std::fprintf(stderr, "ERROR: entry head name read error.\n");
            return EXIT_FAILURE;
        }

        char const * beg_str = reinterpret_cast<char const *>(entry_data.data() + 8);
        char const * end_str = reinterpret_cast<char const *>(memchr(beg_str, 0, head_size - 8));

        if (end_str == nullptr)
            end_str = beg_str + head_size - 8;

        std::string head_name(beg_str, end_str);

        // read data

        if (std::fread(entry_data.data() + head_size, 1, data_size, file) != data_size)
        {
            std::fprintf(stderr, "ERROR: entry data read error.\n");
            return EXIT_FAILURE;
        }

        std::printf(
            "entry %d: %s (%d bytes, unk_02 = %02X)\n", (int)i, head_name.c_str(), (int)data_size, (int)unknown_2);

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

        if (std::fwrite(entry_data.data() + head_size, 1, data_size, output_file) != data_size)
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

int main(int argc, char const * argv[])
{
    if (argc < 2)
    {
        std::fprintf(
            stderr,
            "Usage:\n"
            "  %s dump PACK DIR\n"
            "  %s pack PACK FILES...\n",
            argv[0], argv[0]);

        return EXIT_FAILURE;
    }

    std::string_view subcommand { argv[1] };

    if (subcommand == "pack")
        return Pack(argc - 2, argv + 2);

    if (subcommand == "dump")
        return Dump(argc - 2, argv + 2);

    return EXIT_SUCCESS;
}