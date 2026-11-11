#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <cstdlib>
#include <cstdint>
#include <sys/stat.h>
#include <unistd.h>

#include "disk_image.h"
#include "hfs_filesystem.h"

static const std::string COLOR_RESET  = "\033[0m";
static const std::string COLOR_BOLD   = "\033[1m";
static const std::string COLOR_CYAN   = "\033[1;36m";
static const std::string COLOR_GREEN  = "\033[1;32m";
static const std::string COLOR_YELLOW = "\033[1;33m";
static const std::string COLOR_RED    = "\033[1;31m";
static const std::string COLOR_MAGENTA= "\033[1;35m";

static std::wstring to_wide_str(const std::string& s) {
    return std::wstring(s.begin(), s.end());
}

static std::string format_bytes(uint64_t b) {
    if (b < 1024) return std::to_string(b) + " B";
    double kb = (double)b / 1024.0;
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(1);
    if (kb < 1024.0) {
        ss << kb << " KB";
    } else {
        ss << (kb / 1024.0) << " MB";
    }
    return ss.str();
}



static uint64_t get_file_size(const std::string& path) {
    struct stat st;
    if (stat(path.c_str(), &st) == 0) return (uint64_t)st.st_size;
    return 0;
}

static void print_banner() {
    std::cout << COLOR_CYAN << "========================================================================\n"
              << "       PlusOS Disk Studio & Compiler (Native Linux Edition)             \n"
              << "       Bare-metal OS for Macintosh Plus & Macintosh SE (68000)          \n"
              << "========================================================================" << COLOR_RESET << "\n\n";
}

static void cmd_info(const std::string& imagePath) {
    DiskImage disk;
    std::string err;
    if (!disk.open(to_wide_str(imagePath), true, &err)) {
        std::cerr << COLOR_RED << "Error opening image: " << err << COLOR_RESET << "\n";
        return;
    }

    std::cout << COLOR_BOLD << "Disk Image: " << COLOR_RESET << imagePath << "\n";
    std::cout << "File Size: " << format_bytes(disk.get_file_size()) << " (" << disk.get_total_blocks() << " blocks)\n";
    std::cout << "Partition Map: " << (disk.has_partition_map() ? "Apple Partition Map (APM)" : "Raw / Floppy Image") << "\n\n";

    if (disk.has_partition_map()) {
        std::cout << COLOR_CYAN << "Partition Table:" << COLOR_RESET << "\n";
        std::cout << "Idx  Name                  Type               Start Block   Count\n";
        std::cout << "------------------------------------------------------------------------\n";
        for (const auto& p : disk.get_partitions()) {
            std::cout << std::left << std::setw(5) << p.index
                      << std::left << std::setw(22) << p.name
                      << std::left << std::setw(19) << p.type
                      << std::right << std::setw(11) << p.start_block
                      << std::right << std::setw(11) << p.block_count;
            if (p.is_hfs) std::cout << "  [HFS Volume]";
            std::cout << "\n";
        }
        std::cout << "\n";
    }

    HFSFileSystem fs;
    if (fs.mount(&disk, &err)) {
        std::cout << COLOR_GREEN << "HFS Volume Name: " << COLOR_RESET << fs.get_volume_name() << "\n";
        std::cout << "Volume Capacity: " << format_bytes(fs.get_total_bytes()) << "\n";
        std::cout << "Free Space:      " << format_bytes(fs.get_free_bytes()) << "\n";
        std::cout << "Alloc Block:     " << fs.get_alloc_block_size() << " bytes\n";
        std::cout << "Total Files:     " << fs.get_total_files() << "\n";
        std::cout << "Total Folders:   " << fs.get_total_folders() << "\n";
    } else {
        std::cout << COLOR_YELLOW << "HFS Mount Note: " << err << COLOR_RESET << "\n";
    }
}

static void print_hfs_tree(const HFSFileSystem& fs, uint32_t parent_cnid, const std::string& indent) {
    auto items = fs.get_children(parent_cnid);
    for (const auto& item : items) {
        if (item.is_directory) {
            std::cout << indent << COLOR_CYAN << "[DIR]  " << item.name << "/" << COLOR_RESET << "\n";
            print_hfs_tree(fs, item.cnid, indent + "       ");
        } else {
            std::string typeStr = fourcc_to_str(item.type);
            std::string creatorStr = fourcc_to_str(item.creator);
            std::cout << indent << COLOR_GREEN << "[FILE] " << COLOR_RESET
                      << std::left << std::setw(24) << item.name << " "
                      << std::right << std::setw(9) << format_bytes(item.data_size) << "  "
                      << "Type:" << typeStr << " Creator:" << creatorStr << "\n";
        }
    }
}

static void cmd_ls(const std::string& imagePath) {
    DiskImage disk;
    std::string err;
    if (!disk.open(to_wide_str(imagePath), true, &err)) {
        std::cerr << COLOR_RED << "Error opening image: " << err << COLOR_RESET << "\n";
        return;
    }

    HFSFileSystem fs;
    if (!fs.mount(&disk, &err)) {
        std::cerr << COLOR_RED << "Failed to mount HFS: " << err << COLOR_RESET << "\n";
        return;
    }

    std::cout << COLOR_BOLD << "Volume: " << fs.get_volume_name() << " ("
              << format_bytes(fs.get_free_bytes()) << " free)" << COLOR_RESET << "\n\n";
    print_hfs_tree(fs, 2, "  ");
}

static void cmd_extract(const std::string& imagePath, const std::string& macName, const std::string& outPath) {
    DiskImage disk;
    std::string err;
    if (!disk.open(to_wide_str(imagePath), true, &err)) {
        std::cerr << COLOR_RED << "Error opening image: " << err << COLOR_RESET << "\n";
        return;
    }

    HFSFileSystem fs;
    if (!fs.mount(&disk, &err)) {
        std::cerr << COLOR_RED << "Failed to mount HFS: " << err << COLOR_RESET << "\n";
        return;
    }

    const HFSItem* item = fs.find_by_path(macName);
    if (!item) {
        // Try searching in root
        auto children = fs.get_children(2);
        for (const auto& c : children) {
            if (c.name == macName) {
                item = &c;
                break;
            }
        }
    }

    if (!item) {
        std::cerr << COLOR_RED << "File not found on HFS volume: " << macName << COLOR_RESET << "\n";
        return;
    }

    std::vector<uint8_t> data;
    if (!fs.read_file_fork(*item, false, data)) {
        std::cerr << COLOR_RED << "Failed to read data fork for: " << macName << COLOR_RESET << "\n";
        return;
    }

    std::string savePath = outPath.empty() ? item->name : outPath;
    std::ofstream out(savePath, std::ios::binary);
    if (!out.is_open()) {
        std::cerr << COLOR_RED << "Failed to open output file: " << savePath << COLOR_RESET << "\n";
        return;
    }

    out.write((const char*)data.data(), data.size());
    out.close();
    std::cout << COLOR_GREEN << "Extracted " << data.size() << " bytes to " << savePath << COLOR_RESET << "\n";
}

static void cmd_import(const std::string& imagePath, const std::string& localFile, const std::string& macName) {
    DiskImage disk;
    std::string err;
    if (!disk.open(to_wide_str(imagePath), false, &err)) {
        std::cerr << COLOR_RED << "Error opening image for writing: " << err << COLOR_RESET << "\n";
        return;
    }

    HFSFileSystem fs;
    if (!fs.mount(&disk, &err)) {
        std::cerr << COLOR_RED << "Failed to mount HFS: " << err << COLOR_RESET << "\n";
        return;
    }

    std::ifstream in(localFile, std::ios::binary);
    if (!in.is_open()) {
        std::cerr << COLOR_RED << "Failed to open local file: " << localFile << COLOR_RESET << "\n";
        return;
    }

    std::vector<uint8_t> data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    in.close();

    std::string name = macName;
    if (name.empty()) {
        size_t s = localFile.find_last_of("/\\");
        name = (s != std::string::npos) ? localFile.substr(s + 1) : localFile;
    }

    // FourCC type detection: .pex -> PLOS/PLOS, .txt -> TEXT/ttxt, otherwise ????/????
    uint32_t type = 0x3F3F3F3F;    // '????'
    uint32_t creator = 0x3F3F3F3F; // '????'
    if (name.size() > 4 && name.substr(name.size() - 4) == ".pex") {
        type = 0x504C4F53;    // 'PLOS'
        creator = 0x504C4F53; // 'PLOS'
    } else if (name.size() > 4 && name.substr(name.size() - 4) == ".txt") {
        type = 0x54455854;    // 'TEXT'
        creator = 0x74747874; // 'ttxt'
    }

    if (fs.import_file(2, name, type, creator, data)) {
        std::cout << COLOR_GREEN << "Successfully imported " << name << " (" << data.size() << " bytes) to HFS image." << COLOR_RESET << "\n";
    } else {
        std::cerr << COLOR_RED << "Failed to import file into HFS image." << COLOR_RESET << "\n";
    }
}

static void cmd_inject(const std::string& imagePath, int scsi_id) {
    std::string msg;
    if (HFSFileSystem::inject_into_external_image(to_wide_str(imagePath), (uint32_t)scsi_id, &msg)) {
        std::cout << COLOR_GREEN << "Injection Succeeded: " << msg << COLOR_RESET << "\n";
    } else {
        std::cerr << COLOR_RED << "Injection Failed: " << msg << COLOR_RESET << "\n";
    }
}

static void cmd_build_image(const std::string& machine = "se") {
    std::cout << COLOR_CYAN << "Starting PlusOS hard drive compilation & install pipeline for "
              << (machine == "universal" ? "Universal (any Mac, CPU auto-detected)" :
                  machine == "lc3" ? "Macintosh LC III (68030, 640x480)" :
                  machine == "plus" ? "Macintosh Plus (68000)" :
                  machine == "classic" ? "Macintosh Classic (68000)" : "Macintosh SE (68000)")
              << "...\n" << COLOR_RESET;
    std::string cmd = "make install MACHINE=" + machine;
    int res = system(cmd.c_str());
    if (res == 0) {
        std::cout << COLOR_GREEN << "\n[SUCCESS] PlusOS hard drive image built and installed successfully!\n" << COLOR_RESET;
    } else {
        std::cerr << COLOR_RED << "\n[ERROR] Build failed with exit code " << res << COLOR_RESET << "\n";
    }
}

static void cmd_build_floppy(const std::string& machine = "se") {
    std::cout << COLOR_CYAN << "Starting minimal bootable floppy compilation pipeline (kernel: MACHINE="
              << machine << ")...\n" << COLOR_RESET;
    int res = system(("make floppy MACHINE=" + machine).c_str());
    if (res == 0) {
        std::cout << COLOR_GREEN << "\n[SUCCESS] images/PlusOS_Floppy.dsk created successfully!\n" << COLOR_RESET;
    } else {
        std::cerr << COLOR_RED << "\n[ERROR] Floppy build failed with exit code " << res << COLOR_RESET << "\n";
    }
}

static void cmd_setup_env() {
    std::cout << COLOR_CYAN << "=== Setting Up PlusOS Build Environment & Dependencies ===\n" << COLOR_RESET;
    std::cout << "[1/2] Installing companion Python dependencies (machfs, pyserial, requests, urllib3)...\n";
    int pipRes = system("pip3 install -r requirements.txt || python3 -m pip install -r requirements.txt");
    if (pipRes == 0) {
        std::cout << COLOR_GREEN << "[OK] Python dependencies installed successfully.\n" << COLOR_RESET;
    } else {
        std::cout << COLOR_YELLOW << "[NOTICE] pip install returned code " << pipRes << ". If already satisfied, this is fine.\n" << COLOR_RESET;
    }

    std::cout << "\n[2/2] Checking Motorola 68000 cross-compiler...\n";
    int gccRes = system("which m68k-apple-macos-gcc >/dev/null 2>&1");
    if (gccRes == 0) {
        std::cout << COLOR_GREEN << "[OK] Retro68 m68k-apple-macos-gcc toolchain found.\n" << COLOR_RESET;
    } else {
        std::cout << COLOR_YELLOW << "[NOTICE] m68k-apple-macos-gcc not found in standard PATH.\n"
                  << "         Checking config.mk for Retro68 toolchain path...\n" << COLOR_RESET;
    }
}

static void run_interactive() {
    std::string defaultImage = "images/PlusOS.hda";
    std::string currentMachine = "se";   /* Target Machine, shared by image & floppy builds */
    if (access(defaultImage.c_str(), F_OK) != 0) {
        if (access("BOOTTEST.hda", F_OK) == 0) defaultImage = "BOOTTEST.hda";
    }

    while (true) {
        print_banner();
        std::cout << COLOR_BOLD << "Current Disk Image: " << COLOR_GREEN << defaultImage << COLOR_RESET << "\n\n";
        std::cout << "  1. Explore Disk Image (Partition Map & HFS Hierarchy)\n";
        std::cout << "  2. List All Files (with Sizes & FourCC Types)\n";
        std::cout << "  3. Extract File from Image\n";
        std::cout << "  4. Import File into Image\n";
        std::cout << "  5. Inject 'Return to PlusOS' into External Mac OS Image\n";
        std::cout << "  6. Change Current Disk Image\n";
        std::cout << "  7. Compile & Build Hard Drive Image (make install)\n";
        std::cout << "  8. Build Bootable Floppy (.dsk) (make floppy)\n";
        std::cout << "  9. Setup Build Environment & Pip Dependencies\n";
        std::cout << "  0. Exit\n\n";
        std::cout << COLOR_CYAN << "Select an option [0-9]: " << COLOR_RESET;

        std::string choice;
        if (!std::getline(std::cin, choice)) break;
        if (choice == "0" || choice == "q" || choice == "exit") break;

        if (choice == "1") {
            cmd_info(defaultImage);
        } else if (choice == "2") {
            cmd_ls(defaultImage);
        } else if (choice == "3") {
            std::cout << "Enter Macintosh file name to extract: ";
            std::string macFile;
            std::getline(std::cin, macFile);
            std::cout << "Enter destination local path (leave empty for same name): ";
            std::string locFile;
            std::getline(std::cin, locFile);
            if (!macFile.empty()) cmd_extract(defaultImage, macFile, locFile);
        } else if (choice == "4") {
            std::cout << "Enter local file path to import: ";
            std::string locFile;
            std::getline(std::cin, locFile);
            std::cout << "Enter Macintosh destination file name (leave empty for same): ";
            std::string macFile;
            std::getline(std::cin, macFile);
            if (!locFile.empty()) cmd_import(defaultImage, locFile, macFile);
        } else if (choice == "5") {
            std::cout << "Enter target disk or floppy image path: ";
            std::string injPath;
            std::getline(std::cin, injPath);
            std::cout << "Enter SCSI ID to return to (default: 6): ";
            std::string scsiStr;
            std::getline(std::cin, scsiStr);
            int sid = 6;
            if (!scsiStr.empty()) sid = std::atoi(scsiStr.c_str());
            if (!injPath.empty()) cmd_inject(injPath, sid);
        } else if (choice == "6") {
            std::cout << "Enter new disk image path: ";
            std::string newPath;
            std::getline(std::cin, newPath);
            if (!newPath.empty()) defaultImage = newPath;
        } else if (choice == "7") {
            std::cout << "Target Machine:\n"
                      << "  1. Macintosh SE (8 MHz 68000, ADB) [Default]\n"
                      << "  2. Macintosh Plus (8 MHz 68000, Serial Keyboard)\n"
                      << "  3. Macintosh Classic (8 MHz 68000, ADB)\n"
                      << "  4. Macintosh LC III (25 MHz 68030, 32-bit Bus, 640x480 RGB)\n"
                      << "  5. Universal - ONE image, boots any Mac (auto-detects CPU)\n"
                      << "Select machine [1-5, default=1]: ";
            std::string mChoice;
            std::getline(std::cin, mChoice);
            currentMachine = "se";
            if (mChoice == "2") currentMachine = "plus";
            else if (mChoice == "3") currentMachine = "classic";
            else if (mChoice == "4") currentMachine = "lc3";
            else if (mChoice == "5") currentMachine = "universal";
            cmd_build_image(currentMachine);
        } else if (choice == "8") {
            cmd_build_floppy(currentMachine);
        } else if (choice == "9") {
            cmd_setup_env();
        }

        std::cout << "\nPress Enter to continue...";
        std::string dummy;
        std::getline(std::cin, dummy);
        std::cout << "\n";
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        run_interactive();
        return 0;
    }

    std::string cmd = argv[1];
    if (cmd == "info" && argc >= 3) {
        cmd_info(argv[2]);
    } else if (cmd == "ls" && argc >= 3) {
        cmd_ls(argv[2]);
    } else if (cmd == "extract" && argc >= 4) {
        std::string out = (argc >= 5) ? argv[4] : "";
        cmd_extract(argv[2], argv[3], out);
    } else if (cmd == "import" && argc >= 4) {
        std::string mac = (argc >= 5) ? argv[4] : "";
        cmd_import(argv[2], argv[3], mac);
    } else if (cmd == "inject" && argc >= 3) {
        int scsi_id = (argc >= 4) ? std::atoi(argv[3]) : 6;
        cmd_inject(argv[2], scsi_id);
    } else if (cmd == "build") {
        std::string mach = (argc >= 3) ? argv[2] : "se";
        cmd_build_image(mach);
    } else if (cmd == "floppy") {
        std::string mach = (argc >= 3) ? argv[2] : "se";
        cmd_build_floppy(mach);
    } else if (cmd == "setup") {
        cmd_setup_env();
    } else if (cmd == "--interactive" || cmd == "-i") {
        run_interactive();
    } else {
        std::cout << "Usage:\n"
                  << "  " << argv[0] << "                   (Interactive mode)\n"
                  << "  " << argv[0] << " info <image.hda>\n"
                  << "  " << argv[0] << " ls <image.hda>\n"
                  << "  " << argv[0] << " extract <image.hda> <mac_file> [local_out]\n"
                  << "  " << argv[0] << " import <image.hda> <local_file> [mac_name]\n"
                  << "  " << argv[0] << " inject <image.hda> [scsi_id]\n"
                  << "  " << argv[0] << " build             (Runs make install)\n"
                  << "  " << argv[0] << " floppy            (Runs make floppy)\n"
                  << "  " << argv[0] << " setup             (Installs pip dependencies)\n";
    }
    return 0;
}
