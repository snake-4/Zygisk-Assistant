#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include <sys/sysmacros.h> // For makedev

#include "map_parser.hpp"
#include "logging.hpp"

using namespace Parsers;

map_entry_t::map_entry_t(uintptr_t address_start, uintptr_t address_end, uintptr_t offset, const std::string &perms, const std::string &pathname, dev_t device, ino_t inode)
    : address_start(address_start), address_end(address_end), perms(perms),
      offset(offset), device(device), inode(inode), pathname(pathname) {}

uintptr_t map_entry_t::getAddressStart() const { return address_start; }
uintptr_t map_entry_t::getAddressEnd() const { return address_end; }
const std::string &map_entry_t::getPerms() const { return perms; }
uintptr_t map_entry_t::getOffset() const { return offset; }
dev_t map_entry_t::getDevice() const { return device; }
ino_t map_entry_t::getInode() const { return inode; }
const std::string &map_entry_t::getPathname() const { return pathname; }

const std::vector<map_entry_t> &Parsers::parseSelfMaps(bool cached)
{
    static std::vector<map_entry_t> parser_cache;
    if (cached && !parser_cache.empty())
    {
        return parser_cache;
    }
    parser_cache.clear();

    std::ifstream ifs("/proc/self/maps", std::ifstream::in);
    if (!ifs)
    {
        LOGE("parseSelfMaps could not open /proc/self/maps");
        return parser_cache;
    }

    for (std::string line; std::getline(ifs, line);)
    {
        std::istringstream iss(line);

        uintptr_t address_start, address_end, offset;
        std::string perms;
        std::string pathname;
        ino_t inode;
        int dev_major, dev_minor;
        char dummy_char;

        iss >> std::hex >> address_start >> dummy_char >> address_end >> perms >> offset >> dev_major >> dummy_char >> dev_minor >> std::dec >> inode;

        if (iss.fail())
        {
            LOGE("parseSelfMaps failed to parse line: %s", line.c_str());
            continue;
        }

        // This operation can fail, it doesn't matter as it's an optional field.
        std::getline(iss >> std::ws, pathname);

        parser_cache.emplace_back(map_entry_t(address_start, address_end, offset, perms, pathname, makedev(dev_major, dev_minor), inode));
    }

    return parser_cache;
}
