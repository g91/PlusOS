#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include "hfs_structs.h"
#include "disk_image.h"

struct HFSItem {
    uint32_t cnid = 0;
    uint32_t parent_cnid = 0;
    std::string name;
    bool is_directory = false;
    uint32_t type = 0;         // FourCC
    uint32_t creator = 0;      // FourCC
    uint16_t finder_flags = 0;
    uint32_t data_size = 0;
    uint32_t rsrc_size = 0;
    uint32_t create_date = 0;
    uint32_t modify_date = 0;
    HFSExtentRecord data_extents = {};
    HFSExtentRecord rsrc_extents = {};
    std::vector<HFSItem> children;
};

struct MultiBootEntry {
    std::string name;
    int scsi_id = -1; // 0..7: SCSI ID, -1: PlusOS Shell, -2: PlusOS Desktop
    bool is_default = false;
};

class HFSFileSystem {
public:
    HFSFileSystem();
    ~HFSFileSystem();

    bool mount(DiskImage* disk, std::string* error_msg = nullptr);
    void unmount();
    bool is_mounted() const { return m_disk != nullptr && m_mounted; }

    std::string get_volume_name() const { return m_volume_name; }
    uint64_t get_total_bytes() const;
    uint64_t get_free_bytes() const;
    uint32_t get_alloc_block_size() const { return m_mdb.drAlBlkSiz; }
    uint32_t get_total_files() const { return m_mdb.drFilCnt; }
    uint32_t get_total_folders() const { return m_mdb.drDirCnt; }

    // Directory hierarchy
    const HFSItem* get_root() const { return &m_root_dir; }
    const HFSItem* find_by_cnid(uint32_t cnid) const;
    const HFSItem* find_by_path(const std::string& path) const;
    std::vector<HFSItem> get_children(uint32_t parent_cnid) const;
    const std::map<uint32_t, HFSItem>& get_all_items() const { return m_all_items; }

    // File operations
    bool read_file_fork(const HFSItem& item, bool resource_fork, std::vector<uint8_t>& out_data);
    bool import_file(uint32_t parent_cnid, const std::string& mac_name, uint32_t type, uint32_t creator,
                     const std::vector<uint8_t>& data_fork, const std::vector<uint8_t>& rsrc_fork = {},
                     uint16_t finder_flags = 0);
    bool create_folder(uint32_t parent_cnid, const std::string& folder_name);
    bool delete_item(uint32_t cnid);

    // Multi-Boot Configuration
    bool read_multiboot_config(std::vector<MultiBootEntry>& entries, int& boot_pref);
    bool save_multiboot_config(const std::vector<MultiBootEntry>& entries, int boot_pref);

    // Inject "Return to PlusOS" Classic Mac App
    bool inject_return_to_plusos(uint32_t target_scsi_id, std::string* msg = nullptr);

    // Static standalone injector for any disk image or floppy image
    static bool inject_into_external_image(const std::wstring& image_path, uint32_t target_scsi_id, std::string* msg);

private:
    DiskImage* m_disk;
    bool m_mounted;
    HFSMDB m_mdb;
    std::string m_volume_name;
    std::vector<uint8_t> m_vbm; // Volume Bitmap
    std::vector<uint8_t> m_catalog_data; // Catalog B-Tree File Cache
    HFSItem m_root_dir;
    std::map<uint32_t, HFSItem> m_all_items;

    bool read_mdb();
    bool write_mdb();
    bool read_vbm();
    bool write_vbm();
    bool read_catalog();
    bool build_tree();

    // Allocation Block helpers
    uint32_t alloc_to_sector(uint16_t ablock) const;
    bool allocate_blocks(uint16_t count, std::vector<HFSExtentDescriptor>& out_extents);
    void free_blocks(const HFSExtentRecord& extents);
    bool write_extents_data(const HFSExtentRecord& extents, const std::vector<uint8_t>& data);
    bool read_extents_data(const HFSExtentRecord& extents, uint32_t total_size, std::vector<uint8_t>& data);

    // Catalog manipulation
    bool insert_catalog_record(const HFSCatalogKey& key, const void* record_data, uint16_t record_size);
    bool remove_catalog_record(uint32_t parent_cnid, const std::string& name);
    const HFSItem* find_in_tree(const HFSItem& current, uint32_t cnid) const;
};
