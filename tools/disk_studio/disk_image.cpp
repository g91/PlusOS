#include "disk_image.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <cstring>
#include <cerrno>

static std::string wstr_to_utf8(const std::wstring& w) {
    std::string s;
    for (wchar_t c : w) s.push_back((char)c);
    return s;
}
#endif

DiskImage::DiskImage()
    : m_handle(nullptr), m_read_only(false), m_file_size(0),
      m_has_pm(false), m_active_hfs_index(-1),
      m_hfs_start_block(0), m_hfs_block_count(0)
{
}

DiskImage::~DiskImage()
{
    close();
}

bool DiskImage::open(const std::wstring& path, bool read_only, std::string* error_msg)
{
    close();
    m_path = path;
    m_read_only = read_only;

#ifdef _WIN32
    DWORD access = GENERIC_READ | (read_only ? 0 : GENERIC_WRITE);
    DWORD share = FILE_SHARE_READ | (read_only ? FILE_SHARE_WRITE : 0);

    HANDLE h = CreateFileW(path.c_str(), access, share, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) {
        if (!read_only) {
            // Try opening read-only if write access was denied
            h = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
            if (h != INVALID_HANDLE_VALUE) {
                m_read_only = true;
            }
        }
    }

    if (h == INVALID_HANDLE_VALUE) {
        DWORD err = GetLastError();
        if (error_msg) {
            if (err == ERROR_SHARING_VIOLATION || err == ERROR_LOCK_VIOLATION) {
                *error_msg = "The disk image is locked by another program (e.g. Mini vMac). Please close it first.";
            } else if (err == ERROR_FILE_NOT_FOUND) {
                *error_msg = "The specified file was not found.";
            } else if (err == ERROR_ACCESS_DENIED) {
                *error_msg = "Access denied. Check file permissions.";
            } else {
                *error_msg = "Failed to open disk image (Error code " + std::to_string(err) + ").";
            }
        }
        return false;
    }

    LARGE_INTEGER li;
    if (!GetFileSizeEx(h, &li)) {
        CloseHandle(h);
        if (error_msg) *error_msg = "Could not determine file size.";
        return false;
    }

    m_handle = (void*)h;
    m_file_size = (uint64_t)li.QuadPart;
#else
    std::string u8path = wstr_to_utf8(path);
    int fd = ::open(u8path.c_str(), read_only ? O_RDONLY : O_RDWR);
    if (fd < 0 && !read_only) {
        fd = ::open(u8path.c_str(), O_RDONLY);
        if (fd >= 0) m_read_only = true;
    }

    if (fd < 0) {
        if (error_msg) *error_msg = strerror(errno);
        return false;
    }

    struct stat st;
    if (fstat(fd, &st) != 0) {
        ::close(fd);
        if (error_msg) *error_msg = strerror(errno);
        return false;
    }

    m_handle = (void*)(intptr_t)fd;
    m_file_size = (uint64_t)st.st_size;
#endif

    if (!parse_partition_map(error_msg)) {
        close();
        return false;
    }

    return true;
}

void DiskImage::close()
{
#ifdef _WIN32
    if (m_handle && m_handle != INVALID_HANDLE_VALUE) {
        CloseHandle((HANDLE)m_handle);
    }
#else
    if (m_handle) {
        ::close((int)(intptr_t)m_handle);
    }
#endif
    m_handle = nullptr;
    m_path.clear();
    m_read_only = false;
    m_file_size = 0;
    m_has_pm = false;
    m_partitions.clear();
    m_active_hfs_index = -1;
    m_hfs_start_block = 0;
    m_hfs_block_count = 0;
}

const PartitionInfo* DiskImage::get_active_hfs_partition() const
{
    if (m_active_hfs_index >= 0 && m_active_hfs_index < (int)m_partitions.size()) {
        return &m_partitions[m_active_hfs_index];
    }
    return nullptr;
}

bool DiskImage::read_blocks(uint32_t block, uint32_t count, void* buffer)
{
    if (!m_handle || !buffer || count == 0) return false;

#ifdef _WIN32
    LARGE_INTEGER offset;
    offset.QuadPart = (int64_t)block * 512;

    HANDLE h = (HANDLE)m_handle;
    if (!SetFilePointerEx(h, offset, nullptr, FILE_BEGIN)) return false;

    DWORD bytes_to_read = count * 512;
    DWORD bytes_read = 0;
    if (!ReadFile(h, buffer, bytes_to_read, &bytes_read, nullptr) || bytes_read != bytes_to_read) {
        return false;
    }
    return true;
#else
    int fd = (int)(intptr_t)m_handle;
    off_t offset = (off_t)block * 512;
    if (lseek(fd, offset, SEEK_SET) == (off_t)-1) return false;

    ssize_t bytes_to_read = (ssize_t)count * 512;
    ssize_t bytes_read = ::read(fd, buffer, bytes_to_read);
    return bytes_read == bytes_to_read;
#endif
}

bool DiskImage::write_blocks(uint32_t block, uint32_t count, const void* buffer)
{
    if (!m_handle || !buffer || count == 0 || m_read_only) return false;

#ifdef _WIN32
    LARGE_INTEGER offset;
    offset.QuadPart = (int64_t)block * 512;

    HANDLE h = (HANDLE)m_handle;
    if (!SetFilePointerEx(h, offset, nullptr, FILE_BEGIN)) return false;

    DWORD bytes_to_write = count * 512;
    DWORD bytes_written = 0;
    if (!WriteFile(h, buffer, bytes_to_write, &bytes_written, nullptr) || bytes_written != bytes_to_write) {
        return false;
    }
    return true;
#else
    int fd = (int)(intptr_t)m_handle;
    off_t offset = (off_t)block * 512;
    if (lseek(fd, offset, SEEK_SET) == (off_t)-1) return false;

    ssize_t bytes_to_write = (ssize_t)count * 512;
    ssize_t bytes_written = ::write(fd, buffer, bytes_to_write);
    return bytes_written == bytes_to_write;
#endif
}

bool DiskImage::read_hfs_blocks(uint32_t part_block, uint32_t count, void* buffer)
{
    return read_blocks(m_hfs_start_block + part_block, count, buffer);
}

bool DiskImage::write_hfs_blocks(uint32_t part_block, uint32_t count, const void* buffer)
{
    return write_blocks(m_hfs_start_block + part_block, count, buffer);
}

bool DiskImage::flush()
{
    if (!m_handle || m_read_only) return false;
#ifdef _WIN32
    return FlushFileBuffers((HANDLE)m_handle) != 0;
#else
    return ::fsync((int)(intptr_t)m_handle) == 0;
#endif
}

bool DiskImage::parse_partition_map(std::string* error_msg)
{
    m_partitions.clear();
    m_active_hfs_index = -1;
    m_has_pm = false;

    if (m_file_size < 1024) {
        if (error_msg) *error_msg = "Image is too small to be a valid disk.";
        return false;
    }

    uint8_t sec0[512];
    if (!read_blocks(0, 1, sec0)) {
        if (error_msg) *error_msg = "Failed to read block 0 of image.";
        return false;
    }

    // Check for Apple Driver Descriptor (ER) signature: 0x4552
    if (sec0[0] == 0x45 && sec0[1] == 0x52) {
        m_has_pm = true;
        uint32_t total_map_entries = 0;

        for (uint32_t i = 1; i <= 64; ++i) {
            uint8_t pm_sec[512];
            if (!read_blocks(i, 1, pm_sec)) break;

            ApplePartition* pm = (ApplePartition*)pm_sec;
            if (be16(pm->pmSig) != 0x504D) break; // 'PM'

            if (total_map_entries == 0) {
                total_map_entries = be32(pm->pmMapBlkCnt);
            }

            PartitionInfo info;
            info.index = (int)i;
            info.name = std::string(pm->pmPartName, strnlen(pm->pmPartName, 32));
            info.type = std::string(pm->pmPartType, strnlen(pm->pmPartType, 32));
            info.start_block = be32(pm->pmPyPartStart);
            info.block_count = be32(pm->pmPartBlkCnt);
            info.is_hfs = (info.type == "Apple_HFS");

            m_partitions.push_back(info);

            if (info.is_hfs && m_active_hfs_index < 0) {
                m_active_hfs_index = (int)m_partitions.size() - 1;
                m_hfs_start_block = info.start_block;
                m_hfs_block_count = info.block_count;
            }

            if (total_map_entries > 0 && i >= total_map_entries) break;
        }

        if (m_active_hfs_index >= 0) {
            return true;
        }
    }

    // If no Apple Partition Map found, test for raw unpartitioned HFS (e.g. Floppy image or raw partition)
    // Check block 2 for HFS MDB signature: 0x4244 ('BD')
    uint8_t mdb_sec[512];
    if (read_blocks(2, 1, mdb_sec)) {
        if (mdb_sec[0] == 0x42 && mdb_sec[1] == 0x44) {
            m_has_pm = false;
            m_hfs_start_block = 0;
            m_hfs_block_count = (uint32_t)(m_file_size / 512);

            PartitionInfo info;
            info.index = 1;
            info.name = "Floppy / Raw HFS";
            info.type = "Apple_HFS";
            info.start_block = 0;
            info.block_count = m_hfs_block_count;
            info.is_hfs = true;
            m_partitions.push_back(info);
            m_active_hfs_index = 0;
            return true;
        }
    }

    if (error_msg) {
        *error_msg = "No Apple_HFS partition or valid Macintosh volume found in image.";
    }
    return false;
}
