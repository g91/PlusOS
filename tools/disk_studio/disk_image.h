#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include "hfs_structs.h"

struct PartitionInfo {
    int index;
    std::string name;
    std::string type;
    uint32_t start_block;
    uint32_t block_count;
    bool is_hfs;
};

class DiskImage {
public:
    DiskImage();
    ~DiskImage();

    bool open(const std::wstring& path, bool read_only = false, std::string* error_msg = nullptr);
    void close();
    bool is_open() const { return m_handle != nullptr; }
    bool is_read_only() const { return m_read_only; }
    std::wstring get_path() const { return m_path; }

    uint64_t get_file_size() const { return m_file_size; }
    uint32_t get_total_blocks() const { return (uint32_t)(m_file_size / 512); }

    // Partition map information
    bool has_partition_map() const { return m_has_pm; }
    const std::vector<PartitionInfo>& get_partitions() const { return m_partitions; }
    const PartitionInfo* get_active_hfs_partition() const;
    uint32_t get_hfs_start_block() const { return m_hfs_start_block; }
    uint32_t get_hfs_block_count() const { return m_hfs_block_count; }

    // Raw Block I/O (512-byte blocks, absolute in image)
    bool read_blocks(uint32_t block, uint32_t count, void* buffer);
    bool write_blocks(uint32_t block, uint32_t count, const void* buffer);

    // Partition-relative Block I/O (512-byte blocks, relative to HFS partition)
    bool read_hfs_blocks(uint32_t part_block, uint32_t count, void* buffer);
    bool write_hfs_blocks(uint32_t part_block, uint32_t count, const void* buffer);

    bool flush();

private:
    void* m_handle; // Windows HANDLE
    std::wstring m_path;
    bool m_read_only;
    uint64_t m_file_size;
    bool m_has_pm;
    std::vector<PartitionInfo> m_partitions;
    int m_active_hfs_index;
    uint32_t m_hfs_start_block;
    uint32_t m_hfs_block_count;

    bool parse_partition_map(std::string* error_msg);
};
