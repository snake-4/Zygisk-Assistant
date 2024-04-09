#pragma once
#include <string>
#include <vector>
#include <unordered_map>

#include <mntent.h>

class mount_entry_t
{
public:
    mount_entry_t(::mntent *entry);
    const std::string &getFsName() const;
    const std::string &getMountPoint() const;
    const std::string &getType() const;
    const std::unordered_map<std::string, std::string> &getOptions() const;
    int getDumpFrequency() const;
    int getPassNumber() const;

private:
    std::string fsname, dir, type;
    std::unordered_map<std::string, std::string> opts_map;
    int freq, passno;
};

std::vector<mount_entry_t> parseMountsFromPath(const char *path);
std::unordered_map<std::string, std::string> parseMountOptions(const std::string &input);
