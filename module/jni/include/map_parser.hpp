#pragma once
#include <sstream>
#include <string>
#include <vector>
#include <sys/types.h>

class map_entry_t
{
public:
    map_entry_t(uintptr_t address_start, uintptr_t address_end, uintptr_t offset,
                const std::string &perms, const std::string &pathname, dev_t device, ino_t inode);

    uintptr_t getAddressStart() const;
    uintptr_t getAddressEnd() const;
    const std::string &getPerms() const;
    uintptr_t getOffset() const;
    dev_t getDevice() const;
    ino_t getInode() const;
    const std::string &getPathname() const;

private:
    uintptr_t address_start, address_end, offset;
    std::string perms, pathname;
    dev_t device;
    ino_t inode;
};

std::vector<map_entry_t> parseMapsFromPath(const char *path);
