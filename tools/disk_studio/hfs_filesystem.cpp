#include "hfs_filesystem.h"
#include "return_to_plusos_payload.h"
#include <algorithm>
#include <sstream>
#include <iostream>
#include <ctime>

static uint32_t mac_time_now() {
    // Mac epoch is Jan 1, 1904. Unix epoch is Jan 1, 1970 (2082844800 seconds difference)
    return (uint32_t)(time(nullptr) + 2082844800ULL);
}

HFSFileSystem::HFSFileSystem()
    : m_disk(nullptr), m_mounted(false)
{
    memset(&m_mdb, 0, sizeof(m_mdb));
}

HFSFileSystem::~HFSFileSystem()
{
    unmount();
}

bool HFSFileSystem::mount(DiskImage* disk, std::string* error_msg)
{
    unmount();
    if (!disk || !disk->is_open()) {
        if (error_msg) *error_msg = "Disk image is not open.";
        return false;
    }

    m_disk = disk;

    if (!read_mdb()) {
        if (error_msg) *error_msg = "Failed to read Master Directory Block (MDB). Not an HFS volume.";
        unmount();
        return false;
    }

    if (!read_vbm()) {
        if (error_msg) *error_msg = "Failed to read Volume Bitmap.";
        unmount();
        return false;
    }

    if (!read_catalog()) {
        if (error_msg) *error_msg = "Failed to read Catalog File.";
        unmount();
        return false;
    }

    if (!build_tree()) {
        if (error_msg) *error_msg = "Failed to parse Catalog B-Tree.";
        unmount();
        return false;
    }

    m_mounted = true;
    return true;
}

void HFSFileSystem::unmount()
{
    m_disk = nullptr;
    m_mounted = false;
    memset(&m_mdb, 0, sizeof(m_mdb));
    m_volume_name.clear();
    m_vbm.clear();
    m_catalog_data.clear();
    m_root_dir = HFSItem();
    m_all_items.clear();
}

uint64_t HFSFileSystem::get_total_bytes() const
{
    return (uint64_t)m_mdb.drNmAlBlks * m_mdb.drAlBlkSiz;
}

uint64_t HFSFileSystem::get_free_bytes() const
{
    return (uint64_t)m_mdb.drFreeBks * m_mdb.drAlBlkSiz;
}

bool HFSFileSystem::read_mdb()
{
    uint8_t sec[512];
    if (!m_disk->read_hfs_blocks(2, 1, sec)) return false;

    HFSMDB* raw = (HFSMDB*)sec;
    if (be16(raw->drSigWord) != 0x4244) return false; // 'BD'

    m_mdb.drSigWord = be16(raw->drSigWord);
    m_mdb.drCrDate = be32(raw->drCrDate);
    m_mdb.drLsMod = be32(raw->drLsMod);
    m_mdb.drAtrb = be16(raw->drAtrb);
    m_mdb.drNmFls = be16(raw->drNmFls);
    m_mdb.drVBMSt = be16(raw->drVBMSt);
    m_mdb.drAllocPtr = be16(raw->drAllocPtr);
    m_mdb.drNmAlBlks = be16(raw->drNmAlBlks);
    m_mdb.drAlBlkSiz = be32(raw->drAlBlkSiz);
    m_mdb.drClpSiz = be32(raw->drClpSiz);
    m_mdb.drAlBlSt = be16(raw->drAlBlSt);
    m_mdb.drNxtCNID = be32(raw->drNxtCNID);
    m_mdb.drFreeBks = be16(raw->drFreeBks);

    memcpy(m_mdb.drVN, raw->drVN, 28);
    int vn_len = m_mdb.drVN[0];
    if (vn_len > 27) vn_len = 27;
    m_volume_name = std::string((char*)&m_mdb.drVN[1], vn_len);

    m_mdb.drNmRtDirs = be16(raw->drNmRtDirs);
    m_mdb.drFilCnt = be32(raw->drFilCnt);
    m_mdb.drDirCnt = be32(raw->drDirCnt);

    m_mdb.drXTFlSize = be32(raw->drXTFlSize);
    for (int i = 0; i < 3; ++i) {
        m_mdb.drXTExtRec[i].xdrStABN = be16(raw->drXTExtRec[i].xdrStABN);
        m_mdb.drXTExtRec[i].xdrNumABlks = be16(raw->drXTExtRec[i].xdrNumABlks);
    }

    m_mdb.drCTFlSize = be32(raw->drCTFlSize);
    for (int i = 0; i < 3; ++i) {
        m_mdb.drCTExtRec[i].xdrStABN = be16(raw->drCTExtRec[i].xdrStABN);
        m_mdb.drCTExtRec[i].xdrNumABlks = be16(raw->drCTExtRec[i].xdrNumABlks);
    }

    return true;
}

bool HFSFileSystem::write_mdb()
{
    if (m_disk->is_read_only()) return false;

    uint8_t sec[512];
    if (!m_disk->read_hfs_blocks(2, 1, sec)) return false;

    HFSMDB* raw = (HFSMDB*)sec;
    raw->drLsMod = be32(mac_time_now());
    raw->drNmFls = be16(m_mdb.drNmFls);
    raw->drAllocPtr = be16(m_mdb.drAllocPtr);
    raw->drNxtCNID = be32(m_mdb.drNxtCNID);
    raw->drFreeBks = be16(m_mdb.drFreeBks);
    raw->drNmRtDirs = be16(m_mdb.drNmRtDirs);
    raw->drFilCnt = be32(m_mdb.drFilCnt);
    raw->drDirCnt = be32(m_mdb.drDirCnt);
    raw->drCTFlSize = be32(m_mdb.drCTFlSize);
    for (int i = 0; i < 3; ++i) {
        raw->drCTExtRec[i].xdrStABN = be16(m_mdb.drCTExtRec[i].xdrStABN);
        raw->drCTExtRec[i].xdrNumABlks = be16(m_mdb.drCTExtRec[i].xdrNumABlks);
    }

    // Write primary MDB at sector 2
    if (!m_disk->write_hfs_blocks(2, 1, sec)) return false;

    // Write alternate MDB at sector (count - 2)
    uint32_t total = m_disk->get_hfs_block_count();
    if (total > 4) {
        m_disk->write_hfs_blocks(total - 2, 1, sec);
    }

    return true;
}

bool HFSFileSystem::read_vbm()
{
    uint32_t nbytes = (m_mdb.drNmAlBlks + 7) / 8;
    uint32_t nblocks = (nbytes + 511) / 512;
    m_vbm.resize(nblocks * 512);

    return m_disk->read_hfs_blocks(m_mdb.drVBMSt, nblocks, m_vbm.data());
}

bool HFSFileSystem::write_vbm()
{
    if (m_disk->is_read_only()) return false;
    uint32_t nbytes = (m_mdb.drNmAlBlks + 7) / 8;
    uint32_t nblocks = (nbytes + 511) / 512;
    return m_disk->write_hfs_blocks(m_mdb.drVBMSt, nblocks, m_vbm.data());
}

uint32_t HFSFileSystem::alloc_to_sector(uint16_t ablock) const
{
    uint32_t spb = m_mdb.drAlBlkSiz / 512;
    return m_mdb.drAlBlSt + (uint32_t)ablock * spb;
}

bool HFSFileSystem::read_catalog()
{
    m_catalog_data.clear();
    uint32_t total_bytes = m_mdb.drCTFlSize;
    if (total_bytes == 0) return false;

    m_catalog_data.resize(total_bytes);
    uint32_t spb = m_mdb.drAlBlkSiz / 512;
    uint32_t bytes_read = 0;

    for (int ext = 0; ext < 3; ++ext) {
        uint16_t start_ab = m_mdb.drCTExtRec[ext].xdrStABN;
        uint16_t count_ab = m_mdb.drCTExtRec[ext].xdrNumABlks;
        if (count_ab == 0) break;

        uint32_t sec = alloc_to_sector(start_ab);
        uint32_t nsec = (uint32_t)count_ab * spb;
        uint32_t ext_bytes = nsec * 512;

        uint32_t to_read = (std::min)(ext_bytes, total_bytes - bytes_read);
        uint32_t sec_to_read = (to_read + 511) / 512;

        std::vector<uint8_t> buf(sec_to_read * 512);
        if (!m_disk->read_hfs_blocks(sec, sec_to_read, buf.data())) return false;

        memcpy(&m_catalog_data[bytes_read], buf.data(), to_read);
        bytes_read += to_read;
        if (bytes_read >= total_bytes) break;
    }

    return bytes_read > 0;
}

bool HFSFileSystem::build_tree()
{
    m_all_items.clear();
    if (m_catalog_data.size() < 512) return false;

    // Node 0 is the Header Node
    const uint8_t* node0 = m_catalog_data.data();
    BTHeaderRecord bth;
    bth.bthDepth = be16(*(uint16_t*)(node0 + 14));
    bth.bthRoot = be32(*(uint32_t*)(node0 + 16));
    bth.bthNRecs = be32(*(uint32_t*)(node0 + 20));
    bth.bthFNode = be32(*(uint32_t*)(node0 + 24));
    bth.bthLNode = be32(*(uint32_t*)(node0 + 28));
    bth.bthNodeSize = be16(*(uint16_t*)(node0 + 32));
    if (bth.bthNodeSize == 0) bth.bthNodeSize = 512;

    // Initialize Root Directory (CNID 2)
    m_root_dir = HFSItem();
    m_root_dir.cnid = 2;
    m_root_dir.parent_cnid = 1;
    m_root_dir.name = m_volume_name;
    m_root_dir.is_directory = true;
    m_all_items[2] = m_root_dir;

    // Walk leaf chain starting from bthFNode
    uint32_t cur_node = bth.bthFNode;
    uint32_t max_nodes = (uint32_t)(m_catalog_data.size() / bth.bthNodeSize);

    while (cur_node != 0 && cur_node < max_nodes) {
        const uint8_t* nd = &m_catalog_data[cur_node * bth.bthNodeSize];
        BTNodeDescriptor desc;
        desc.ndFLink = be32(*(uint32_t*)(nd + 0));
        desc.ndBLink = be32(*(uint32_t*)(nd + 4));
        desc.ndType = (int8_t)nd[8];
        desc.ndNHeight = nd[9];
        desc.ndNRecs = be16(*(uint16_t*)(nd + 10));

        if (desc.ndType == -1 || desc.ndType == (int8_t)0xFF) { // Leaf node
            for (uint16_t i = 0; i < desc.ndNRecs; ++i) {
                uint16_t off = be16(*(uint16_t*)(nd + bth.bthNodeSize - 2 * (i + 1)));
                if (off + 2 >= bth.bthNodeSize) continue;

                uint8_t klen = nd[off];
                if (klen == 0 || off + 1 + klen >= bth.bthNodeSize) continue;

                uint32_t par_id = be32(*(uint32_t*)(nd + off + 2));
                uint8_t nlen = nd[off + 6];
                std::string item_name((char*)&nd[off + 7], nlen);

                uint16_t rec_off = (off + 1 + klen + 1) & ~1; // 2-byte aligned
                if (rec_off + 2 > bth.bthNodeSize) continue;

                uint8_t rtype = nd[rec_off];

                if (rtype == 1) { // Directory Record (cdrDirRec)
                    const HFSCatalogFolder* cf = (const HFSCatalogFolder*)(nd + rec_off);
                    uint32_t dir_id = be32(cf->dirDirID);

                    HFSItem item;
                    item.cnid = dir_id;
                    item.parent_cnid = par_id;
                    item.name = item_name;
                    item.is_directory = true;
                    item.type = str_to_fourcc("fold");
                    item.create_date = be32(cf->dirCrDat);
                    item.modify_date = be32(cf->dirMdDat);

                    m_all_items[dir_id] = item;
                }
                else if (rtype == 2) { // File Record (cdrFilRec)
                    const HFSCatalogFile* fl = (const HFSCatalogFile*)(nd + rec_off);
                    uint32_t file_id = be32(fl->filFlNum);

                    HFSItem item;
                    item.cnid = file_id;
                    item.parent_cnid = par_id;
                    item.name = item_name;
                    item.is_directory = false;
                    item.type = be32(fl->fdType);
                    item.creator = be32(fl->fdCreator);
                    item.finder_flags = be16(fl->fdFlags);
                    item.data_size = be32(fl->filLgLen);
                    item.rsrc_size = be32(fl->filRLgLen);
                    item.create_date = be32(fl->filCrDat);
                    item.modify_date = be32(fl->filMdDat);

                    for (int e = 0; e < 3; ++e) {
                        item.data_extents[e].xdrStABN = be16(fl->filExtRec[e].xdrStABN);
                        item.data_extents[e].xdrNumABlks = be16(fl->filExtRec[e].xdrNumABlks);
                        item.rsrc_extents[e].xdrStABN = be16(fl->filRExtRec[e].xdrStABN);
                        item.rsrc_extents[e].xdrNumABlks = be16(fl->filRExtRec[e].xdrNumABlks);
                    }

                    m_all_items[file_id] = item;
                }
            }
        }

        if (desc.ndFLink == 0 || desc.ndFLink == cur_node) break;
        cur_node = desc.ndFLink;
    }

    // Populate children lists
    for (auto& pair : m_all_items) {
        pair.second.children.clear();
    }
    for (const auto& pair : m_all_items) {
        if (pair.first == 2) continue; // Skip root as child of itself
        uint32_t pid = pair.second.parent_cnid;
        auto pit = m_all_items.find(pid);
        if (pit != m_all_items.end()) {
            pit->second.children.push_back(pair.second);
        } else {
            m_all_items[2].children.push_back(pair.second);
        }
    }

    m_root_dir = m_all_items[2];
    return true;
}

const HFSItem* HFSFileSystem::find_by_cnid(uint32_t cnid) const
{
    auto it = m_all_items.find(cnid);
    if (it != m_all_items.end()) return &it->second;
    return nullptr;
}

std::vector<HFSItem> HFSFileSystem::get_children(uint32_t parent_cnid) const
{
    std::vector<HFSItem> res;
    for (const auto& pair : m_all_items) {
        if (pair.first != 2 && pair.second.parent_cnid == parent_cnid) {
            res.push_back(pair.second);
        }
    }
    std::sort(res.begin(), res.end(), [](const HFSItem& a, const HFSItem& b) {
        if (a.is_directory != b.is_directory) return a.is_directory;
        return a.name < b.name;
    });
    return res;
}

const HFSItem* HFSFileSystem::find_by_path(const std::string& path) const
{
    if (path.empty() || path == ":" || path == "/") return &m_root_dir;

    std::string clean = path;
    if (clean.front() == ':' || clean.front() == '/') clean.erase(clean.begin());

    std::stringstream ss(clean);
    std::string seg;
    const HFSItem* cur = &m_root_dir;

    while (std::getline(ss, seg, ':')) {
        if (seg.empty()) continue;
        bool found = false;
        for (const auto& child : cur->children) {
            if (child.name == seg) {
                cur = &child;
                found = true;
                break;
            }
        }
        if (!found) return nullptr;
    }

    return cur;
}

bool HFSFileSystem::read_extents_data(const HFSExtentRecord& extents, uint32_t total_size, std::vector<uint8_t>& data)
{
    data.clear();
    if (total_size == 0) return true;

    data.resize(total_size);
    uint32_t spb = m_mdb.drAlBlkSiz / 512;
    uint32_t bytes_read = 0;

    for (int i = 0; i < 3; ++i) {
        uint16_t start_ab = extents[i].xdrStABN;
        uint16_t count_ab = extents[i].xdrNumABlks;
        if (count_ab == 0) break;

        uint32_t sec = alloc_to_sector(start_ab);
        uint32_t nsec = (uint32_t)count_ab * spb;
        uint32_t ext_bytes = nsec * 512;

        uint32_t to_read = (std::min)(ext_bytes, total_size - bytes_read);
        uint32_t sec_to_read = (to_read + 511) / 512;

        std::vector<uint8_t> buf(sec_to_read * 512);
        if (!m_disk->read_hfs_blocks(sec, sec_to_read, buf.data())) return false;

        memcpy(&data[bytes_read], buf.data(), to_read);
        bytes_read += to_read;
        if (bytes_read >= total_size) break;
    }

    return bytes_read >= total_size;
}

bool HFSFileSystem::read_file_fork(const HFSItem& item, bool resource_fork, std::vector<uint8_t>& out_data)
{
    if (item.is_directory) return false;
    if (resource_fork) {
        return read_extents_data(item.rsrc_extents, item.rsrc_size, out_data);
    } else {
        return read_extents_data(item.data_extents, item.data_size, out_data);
    }
}

bool HFSFileSystem::allocate_blocks(uint16_t count, std::vector<HFSExtentDescriptor>& out_extents)
{
    out_extents.clear();
    if (count == 0) return true;
    if (m_mdb.drFreeBks < count) return false;

    uint16_t needed = count;
    uint16_t cur_ab = m_mdb.drAllocPtr;
    uint16_t total_ab = m_mdb.drNmAlBlks;

    while (needed > 0 && out_extents.size() < 3) {
        // Find next free run
        uint16_t run_start = 0xFFFF;
        uint16_t run_len = 0;

        for (uint32_t i = 0; i < total_ab; ++i) {
            uint16_t ab = (cur_ab + i) % total_ab;
            uint32_t byte_idx = ab >> 3;
            uint8_t bit_mask = 0x80 >> (ab & 7);

            if (!(m_vbm[byte_idx] & bit_mask)) { // Free block
                if (run_len == 0) run_start = ab;
                run_len++;
                if (run_len == needed) break;
            } else {
                if (run_len > 0) break; // End of run
            }
        }

        if (run_len == 0) return false; // Disk full / fragmented

        HFSExtentDescriptor ext;
        ext.xdrStABN = run_start;
        ext.xdrNumABlks = run_len;
        out_extents.push_back(ext);

        // Mark blocks as used in bitmap
        for (uint16_t b = 0; b < run_len; ++b) {
            uint16_t ab = run_start + b;
            m_vbm[ab >> 3] |= (0x80 >> (ab & 7));
        }

        needed -= run_len;
        cur_ab = (run_start + run_len) % total_ab;
    }

    if (needed > 0) return false;

    m_mdb.drFreeBks -= count;
    m_mdb.drAllocPtr = cur_ab;
    return true;
}

void HFSFileSystem::free_blocks(const HFSExtentRecord& extents)
{
    for (int i = 0; i < 3; ++i) {
        uint16_t start = extents[i].xdrStABN;
        uint16_t num = extents[i].xdrNumABlks;
        if (num == 0) break;

        for (uint16_t b = 0; b < num; ++b) {
            uint16_t ab = start + b;
            m_vbm[ab >> 3] &= ~(0x80 >> (ab & 7));
        }
        m_mdb.drFreeBks += num;
    }
}

bool HFSFileSystem::write_extents_data(const HFSExtentRecord& extents, const std::vector<uint8_t>& data)
{
    if (data.empty()) return true;

    uint32_t spb = m_mdb.drAlBlkSiz / 512;
    uint32_t bytes_written = 0;

    for (int i = 0; i < 3; ++i) {
        uint16_t start_ab = extents[i].xdrStABN;
        uint16_t count_ab = extents[i].xdrNumABlks;
        if (count_ab == 0) break;

        uint32_t sec = alloc_to_sector(start_ab);
        uint32_t nsec = (uint32_t)count_ab * spb;
        uint32_t ext_bytes = nsec * 512;

        std::vector<uint8_t> buf(ext_bytes, 0);
        uint32_t to_write = (std::min)(ext_bytes, (uint32_t)data.size() - bytes_written);
        memcpy(buf.data(), &data[bytes_written], to_write);

        if (!m_disk->write_hfs_blocks(sec, nsec, buf.data())) return false;
        bytes_written += to_write;
        if (bytes_written >= data.size()) break;
    }

    return bytes_written >= data.size();
}

bool HFSFileSystem::import_file(uint32_t parent_cnid, const std::string& mac_name, uint32_t type, uint32_t creator,
                                const std::vector<uint8_t>& data_fork, const std::vector<uint8_t>& rsrc_fork,
                                uint16_t finder_flags)
{
    if (m_disk->is_read_only()) return false;

    // Delete existing file with the same name if present
    delete_item(parent_cnid); // Check by name inside parent
    for (const auto& item : m_all_items) {
        if (item.second.parent_cnid == parent_cnid && item.second.name == mac_name) {
            delete_item(item.first);
            break;
        }
    }

    uint32_t blk_size = m_mdb.drAlBlkSiz;
    uint16_t data_blocks = (uint16_t)((data_fork.size() + blk_size - 1) / blk_size);
    uint16_t rsrc_blocks = (uint16_t)((rsrc_fork.size() + blk_size - 1) / blk_size);

    std::vector<HFSExtentDescriptor> data_ext;
    std::vector<HFSExtentDescriptor> rsrc_ext;

    if (!allocate_blocks(data_blocks, data_ext)) return false;
    if (!allocate_blocks(rsrc_blocks, rsrc_ext)) {
        // Rollback data blocks
        HFSExtentRecord rollback = {};
        for (size_t i = 0; i < data_ext.size(); ++i) rollback[i] = data_ext[i];
        free_blocks(rollback);
        return false;
    }

    HFSExtentRecord d_rec = {}, r_rec = {};
    for (size_t i = 0; i < data_ext.size() && i < 3; ++i) d_rec[i] = data_ext[i];
    for (size_t i = 0; i < rsrc_ext.size() && i < 3; ++i) r_rec[i] = rsrc_ext[i];

    if (!write_extents_data(d_rec, data_fork)) return false;
    if (!write_extents_data(r_rec, rsrc_fork)) return false;

    // Construct Catalog File Record
    HFSCatalogKey key = {};
    key.ckrKeyLen = (uint8_t)(1 + 4 + 1 + mac_name.length());
    key.ckrParID = be32(parent_cnid);
    key.ckrCName[0] = (uint8_t)mac_name.length();
    memcpy(&key.ckrCName[1], mac_name.data(), mac_name.length());

    HFSCatalogFile fil = {};
    fil.cdrType = 2; // cdrFilRec
    fil.cdrResrv2 = 0;
    fil.filFlags = 2; // record used
    fil.fdType = be32(type);
    fil.fdCreator = be32(creator);
    fil.fdFlags = be16(finder_flags);
    fil.filFlNum = be32(m_mdb.drNxtCNID++);
    fil.filLgLen = be32((uint32_t)data_fork.size());
    fil.filPyLen = be32((uint32_t)data_blocks * blk_size);
    fil.filRLgLen = be32((uint32_t)rsrc_fork.size());
    fil.filRPyLen = be32((uint32_t)rsrc_blocks * blk_size);
    fil.filCrDat = be32(mac_time_now());
    fil.filMdDat = fil.filCrDat;

    for (int i = 0; i < 3; ++i) {
        fil.filExtRec[i].xdrStABN = be16(d_rec[i].xdrStABN);
        fil.filExtRec[i].xdrNumABlks = be16(d_rec[i].xdrNumABlks);
        fil.filRExtRec[i].xdrStABN = be16(r_rec[i].xdrStABN);
        fil.filRExtRec[i].xdrNumABlks = be16(r_rec[i].xdrNumABlks);
    }

    if (!insert_catalog_record(key, &fil, sizeof(fil))) return false;

    m_mdb.drFilCnt++;
    if (parent_cnid == 2) m_mdb.drNmFls++;

    write_vbm();
    write_mdb();
    m_disk->flush();

    build_tree();
    return true;
}

bool HFSFileSystem::insert_catalog_record(const HFSCatalogKey& key, const void* record_data, uint16_t record_size)
{
    if (m_catalog_data.size() < 512) return false;

    // Insert record into first available leaf node with space
    uint32_t node_size = 512;
    uint32_t cur_node = be32(*(uint32_t*)(m_catalog_data.data() + 24)); // bthFNode

    uint16_t total_rec_size = (uint16_t)(((1 + key.ckrKeyLen + 1) & ~1) + record_size);

    while (cur_node != 0) {
        uint8_t* nd = &m_catalog_data[cur_node * node_size];
        int8_t ntype = (int8_t)nd[8];
        uint16_t nrecs = be16(*(uint16_t*)(nd + 10));

        if (ntype == -1 || ntype == (int8_t)0xFF) { // Leaf node
            uint16_t last_rec_off = be16(*(uint16_t*)(nd + node_size - 2 * nrecs));
            uint16_t free_space_start = (nrecs == 0) ? 14 : be16(*(uint16_t*)(nd + node_size - 2 * (nrecs + 1)));
            uint16_t table_start = (uint16_t)(node_size - 2 * (nrecs + 2));

            // Check if this node has space for record + offset table entry
            if (table_start > free_space_start && (table_start - free_space_start) >= (total_rec_size + 2)) {
                // Insert at free_space_start
                uint16_t new_off = free_space_start;
                uint16_t key_len = 1 + key.ckrKeyLen;
                memcpy(nd + new_off, &key, key_len);

                uint16_t data_off = (uint16_t)((new_off + key_len + 1) & ~1);
                memcpy(nd + data_off, record_data, record_size);

                uint16_t next_free = data_off + record_size;

                // Update offset table
                nrecs++;
                *(uint16_t*)(nd + 10) = be16(nrecs);
                *(uint16_t*)(nd + node_size - 2 * nrecs) = be16(new_off);
                *(uint16_t*)(nd + node_size - 2 * (nrecs + 1)) = be16(next_free);

                // Write node back to disk
                uint32_t spb = m_mdb.drAlBlkSiz / 512;
                uint16_t cat_ab = m_mdb.drCTExtRec[0].xdrStABN;
                uint32_t cat_sec = alloc_to_sector(cat_ab) + cur_node * (node_size / 512);

                return m_disk->write_hfs_blocks(cat_sec, node_size / 512, nd);
            }
        }

        uint32_t flink = be32(*(uint32_t*)(nd + 0));
        if (flink == 0 || flink == cur_node) break;
        cur_node = flink;
    }

    return false;
}

bool HFSFileSystem::delete_item(uint32_t cnid)
{
    if (m_disk->is_read_only()) return false;
    auto it = m_all_items.find(cnid);
    if (it == m_all_items.end()) return false;

    // Free data and resource blocks
    free_blocks(it->second.data_extents);
    free_blocks(it->second.rsrc_extents);

    remove_catalog_record(it->second.parent_cnid, it->second.name);

    if (it->second.is_directory) {
        if (m_mdb.drDirCnt > 0) m_mdb.drDirCnt--;
    } else {
        if (m_mdb.drFilCnt > 0) m_mdb.drFilCnt--;
        if (it->second.parent_cnid == 2 && m_mdb.drNmFls > 0) m_mdb.drNmFls--;
    }

    write_vbm();
    write_mdb();
    m_disk->flush();

    build_tree();
    return true;
}

bool HFSFileSystem::remove_catalog_record(uint32_t parent_cnid, const std::string& name)
{
    uint32_t node_size = 512;
    uint32_t cur_node = be32(*(uint32_t*)(m_catalog_data.data() + 24)); // bthFNode

    while (cur_node != 0) {
        uint8_t* nd = &m_catalog_data[cur_node * node_size];
        int8_t ntype = (int8_t)nd[8];
        uint16_t nrecs = be16(*(uint16_t*)(nd + 10));

        if (ntype == -1 || ntype == (int8_t)0xFF) {
            for (uint16_t i = 0; i < nrecs; ++i) {
                uint16_t off = be16(*(uint16_t*)(nd + node_size - 2 * (i + 1)));
                uint32_t par = be32(*(uint32_t*)(nd + off + 2));
                uint8_t nlen = nd[off + 6];
                std::string iname((char*)&nd[off + 7], nlen);

                if (par == parent_cnid && iname == name) {
                    // Mark key length 0 (deleted)
                    nd[off] = 0;
                    uint16_t cat_ab = m_mdb.drCTExtRec[0].xdrStABN;
                    uint32_t cat_sec = alloc_to_sector(cat_ab) + cur_node * (node_size / 512);
                    m_disk->write_hfs_blocks(cat_sec, node_size / 512, nd);
                    return true;
                }
            }
        }

        uint32_t flink = be32(*(uint32_t*)(nd + 0));
        if (flink == 0 || flink == cur_node) break;
        cur_node = flink;
    }
    return false;
}

bool HFSFileSystem::read_multiboot_config(std::vector<MultiBootEntry>& entries, int& boot_pref)
{
    entries.clear();
    boot_pref = 2; // Default to Boot Menu

    // Look for "multiboot.cfg" in root directory
    const HFSItem* cfg_item = nullptr;
    for (const auto& child : m_root_dir.children) {
        if (child.name == "multiboot.cfg" || child.name == "boot.cfg") {
            cfg_item = &child;
            break;
        }
    }

    if (cfg_item) {
        std::vector<uint8_t> data;
        if (read_file_fork(*cfg_item, false, data) && !data.empty()) {
            std::string content((char*)data.data(), data.size());
            std::stringstream ss(content);
            std::string line;
            std::string cur_name;
            int cur_id = -999;

            while (std::getline(ss, line)) {
                // Trim
                while (!line.empty() && (line.front() == ' ' || line.front() == '\t' || line.front() == '\r')) line.erase(line.begin());
                while (!line.empty() && (line.back() == ' ' || line.back() == '\t' || line.back() == '\r')) line.pop_back();

                if (line.empty() || line[0] == '#' || line[0] == ';') continue;

                if (line[0] == '[') {
                    if (!cur_name.empty() && cur_id != -999) {
                        MultiBootEntry e;
                        e.name = cur_name;
                        e.scsi_id = cur_id;
                        entries.push_back(e);
                    }
                    cur_name.clear();
                    cur_id = -999;
                    continue;
                }

                auto eq = line.find('=');
                if (eq != std::string::npos) {
                    std::string k = line.substr(0, eq);
                    std::string v = line.substr(eq + 1);
                    while (!k.empty() && (k.back() == ' ' || k.back() == '\t')) k.pop_back();
                    while (!v.empty() && (v.front() == ' ' || v.front() == '\t')) v.erase(v.begin());

                    if (k == "name" || k == "title" || k == "label") cur_name = v;
                    else if (k == "id" || k == "scsi_id" || k == "target") {
                        if (v == "shell" || v == "Shell") cur_id = -1;
                        else if (v == "desktop" || v == "Desktop") cur_id = -2;
                        else cur_id = std::stoi(v);
                    }
                    else if (k == "boot_pref" || k == "default_boot") {
                        boot_pref = std::stoi(v);
                    }
                    continue;
                }

                auto comma = line.find(',');
                if (comma != std::string::npos) {
                    std::string nm = line.substr(0, comma);
                    std::string id_str = line.substr(comma + 1);
                    while (!nm.empty() && (nm.back() == ' ' || nm.back() == '\t')) nm.pop_back();
                    while (!id_str.empty() && (id_str.front() == ' ' || id_str.front() == '\t')) id_str.erase(id_str.begin());

                    int sid = -1;
                    if (id_str == "shell" || id_str == "Shell") sid = -1;
                    else if (id_str == "desktop" || id_str == "Desktop") sid = -2;
                    else sid = std::stoi(id_str);

                    MultiBootEntry e;
                    e.name = nm;
                    e.scsi_id = sid;
                    entries.push_back(e);
                }
            }

            if (!cur_name.empty() && cur_id != -999) {
                MultiBootEntry e;
                e.name = cur_name;
                e.scsi_id = cur_id;
                entries.push_back(e);
            }
        }
    }

    if (entries.empty()) {
        // Fallback default entries
        entries = {
            { "System 7.5.5", 3, false },
            { "System 7.1", 2, false },
            { "System 6.0.8", 1, false },
            { "PlusOS Shell", -1, true },
            { "PlusOS Desktop", -2, false }
        };
    }

    return true;
}

bool HFSFileSystem::save_multiboot_config(const std::vector<MultiBootEntry>& entries, int boot_pref)
{
    std::stringstream ss;
    ss << "# PlusOS Multi-Boot Configuration\r\n";
    ss << "# Configured via PlusOS Disk Studio\r\n\r\n";
    ss << "boot_pref=" << boot_pref << "\r\n\r\n";

    for (const auto& e : entries) {
        ss << "[entry]\r\n";
        ss << "name=" << e.name << "\r\n";
        if (e.scsi_id == -1) ss << "id=shell\r\n\r\n";
        else if (e.scsi_id == -2) ss << "id=desktop\r\n\r\n";
        else ss << "id=" << e.scsi_id << "\r\n\r\n";
    }

    std::string s = ss.str();
    std::vector<uint8_t> data(s.begin(), s.end());

    // Import into Root directory (CNID 2)
    return import_file(2, "multiboot.cfg", str_to_fourcc("TEXT"), str_to_fourcc("ttxt"), data);
}

bool HFSFileSystem::inject_return_to_plusos(uint32_t target_scsi_id, std::string* msg)
{
    if (m_disk->is_read_only()) {
        if (msg) *msg = "Cannot inject: disk image is opened read-only.";
        return false;
    }

    // Locate or create ":Desktop" folder
    uint32_t desktop_cnid = 2; // default root
    for (const auto& child : m_root_dir.children) {
        if (child.name == "Desktop" && child.is_directory) {
            desktop_cnid = child.cnid;
            break;
        }
    }

    // Prepare Resource Fork payload
    std::vector<uint8_t> rf(MacAppPayload::RSRC_FORK_DATA,
                           MacAppPayload::RSRC_FORK_DATA + MacAppPayload::RSRC_FORK_SIZE);

    // If SCSI ID is customized, patch the target refNum in CODE 1:
    // refNum for SCSI ID N is -(33 + N) = 0xFFDF - N
    uint16_t custom_refnum = (uint16_t)(0xFFDF - (target_scsi_id & 7));
    // Patch inside CODE 1 (search for 0xFF, 0xD9 and replace with custom_refnum)
    for (size_t i = 0; i + 1 < rf.size(); ++i) {
        if (rf[i] == 0xFF && rf[i + 1] == 0xD9) {
            rf[i] = (uint8_t)(custom_refnum >> 8);
            rf[i + 1] = (uint8_t)(custom_refnum & 0xFF);
            break;
        }
    }

    std::vector<uint8_t> empty_data;
    uint16_t bundle_flags = 0x2000; // Has Bundle bit for Finder to display icon

    // Inject into Desktop
    bool ok = import_file(desktop_cnid, MacAppPayload::APP_NAME,
                          str_to_fourcc("APPL"), str_to_fourcc("PLOS"),
                          empty_data, rf, bundle_flags);

    if (ok) {
        if (msg) {
            *msg = "Successfully injected '" + std::string(MacAppPayload::APP_NAME) +
                   "' (target SCSI ID " + std::to_string(target_scsi_id) + ") onto Macintosh Desktop!";
        }
        return true;
    } else {
        if (msg) *msg = "Failed to inject application into HFS filesystem.";
        return false;
    }
}

bool HFSFileSystem::inject_into_external_image(const std::wstring& image_path, uint32_t target_scsi_id, std::string* msg)
{
    DiskImage img;
    std::string err;
    if (!img.open(image_path, false, &err)) {
        if (msg) *msg = "Could not open target image: " + err;
        return false;
    }

    HFSFileSystem fs;
    if (!fs.mount(&img, &err)) {
        if (msg) *msg = "Could not mount HFS on target image: " + err;
        return false;
    }

    return fs.inject_return_to_plusos(target_scsi_id, msg);
}
