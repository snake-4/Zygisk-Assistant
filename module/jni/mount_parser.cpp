#include <string>
#include <sstream>
#include <vector>
#include <unordered_map>

#include <mntent.h>

#include "mount_parser.hpp"
#include "logging.hpp"
#include "utils.hpp"

mount_entry_t::mount_entry_t(::mntent *entry)
    : fsname(entry->mnt_fsname), dir(entry->mnt_dir), type(entry->mnt_type), freq(entry->mnt_freq), passno(entry->mnt_passno)
{
    opts_map = parseMountOptions(entry->mnt_opts);
}

const std::string &mount_entry_t::getFsName() const { return fsname; }
const std::string &mount_entry_t::getMountPoint() const { return dir; }
const std::string &mount_entry_t::getType() const { return type; }
const std::unordered_map<std::string, std::string> &mount_entry_t::getOptions() const { return opts_map; }
int mount_entry_t::getDumpFrequency() const { return freq; }
int mount_entry_t::getPassNumber() const { return passno; }

const std::vector<mount_entry_t> &parseSelfMounts(bool cached)
{
    static std::vector<mount_entry_t> parser_cache;
    if (cached && !parser_cache.empty())
    {
        return parser_cache;
    }
    parser_cache.clear();

    FILE *file;
    ASSERT_EXIT("parseSelfMounts", (file = setmntent("/proc/self/mounts", "r")) != NULL, return parser_cache);

    struct mntent *entry;
    while ((entry = getmntent(file)) != NULL)
    {
        parser_cache.emplace_back(mount_entry_t(entry));
    }

    endmntent(file);
    return parser_cache;
}

std::unordered_map<std::string, std::string> parseMountOptions(const std::string &input)
{
    std::unordered_map<std::string, std::string> ret;
    std::istringstream iss(input);
    std::string token;
    while (std::getline(iss, token, ','))
    {
        std::istringstream tokenStream(token);
        std::string key, value;

        if (std::getline(tokenStream, key, '='))
        {
            std::getline(tokenStream, value); // Put what's left in the stream to value, could be empty
            ret[key] = value;
        }
    }
    return ret;
}
