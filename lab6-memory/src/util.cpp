#include <cstdint>
#include <fcntl.h>
#include <format>
#include <fstream>
#include <iostream>
#include <sstream>
#include <unistd.h>
#include <vector>

struct pagemap_entry_t {
    uint64_t pfn : 55;
    unsigned int soft_dirty : 1;
    unsigned int file_page : 1;
    unsigned int swapped : 1;
    unsigned int present : 1;
};

static inline auto get_pagemap_entry(int pagemap_fd, uintptr_t page_number) -> pagemap_entry_t {
    pagemap_entry_t entry;
    size_t nread = 0;
    uint64_t data;
    while (nread < sizeof(data)) {
        ssize_t result = pread(pagemap_fd, ((uint8_t *)&data) + nread, sizeof(data) - nread, page_number * sizeof(data) + nread);
        if (result <= 0) {
            std::cerr << "Failed to read pagemap file" << std::endl;
            abort();
        }
        nread += result;
    }

    entry.pfn = data & ((1ull << 55) - 1);
    entry.soft_dirty = (data >> 55) & 1;
    entry.file_page = (data >> 61) & 1;
    entry.swapped = (data >> 62) & 1;
    entry.present = (data >> 63) & 1;
    return entry;
}

int main() {
    pid_t pid;
    std::cout << "Please input pid: " << std::endl;
    std::cin >> pid;

    std::string pagemap_path = std::format("/proc/{}/pagemap", pid);
    std::cout << pagemap_path << std::endl;

    int pagemap_fd = open(pagemap_path.c_str(), O_RDONLY);
    if (pagemap_fd == -1) {
        std::cerr << "Failed to open pagemap file" << std::endl;
        abort();
    }

    auto get_physical_address = [&](size_t virtual_address) -> void {
        const size_t page_size = sysconf(_SC_PAGESIZE);

        const size_t page_number = virtual_address / page_size, offset = virtual_address % page_size;

        const pagemap_entry_t entry = get_pagemap_entry(pagemap_fd, page_number);

        if (!entry.present) {
            std::cerr << "Page not present" << std::endl;
            return;
        }

        std::cout
            << std::format("frame_number = 0x{:016x}, soft_dirty = {}, file_page = {}, swapped = {}, present = {}",
                           static_cast<uint64_t>(entry.pfn),
                           static_cast<bool>(entry.soft_dirty),
                           static_cast<bool>(entry.file_page),
                           static_cast<bool>(entry.swapped),
                           static_cast<bool>(entry.present))
            << std::endl;

        const size_t frame_number = entry.pfn;
        const size_t physical_address = frame_number * page_size + offset;

        std::cout << std::format("Physical Address: 0x{:016x}\n", physical_address) << std::endl;
        return;
    };

    while (true) {
        size_t virtual_address;
        std::cout << "Please input virtual address: " << std::endl;
        std::cin >> std::hex >> virtual_address;
        get_physical_address(virtual_address);
    }

    close(pagemap_fd);
    return 0;
}
