#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <sys/types.h>
#include <sys/sysmacros.h>

#include "mountinfo_parser.hpp"
#include "logging.hpp"

using namespace Parsers;

static std::unordered_map<std::string, std::string> parseMountOptions(const std::string &input)
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

mountinfo_entry::mountinfo_entry(int mount_id, int parent_id, dev_t device,
                                     const std::string &root, const std::string &mount_point,
                                     const std::string &mount_options, const std::string &optional_fields,
                                     const std::string &filesystem_type, const std::string &mount_source,
                                     const std::string &super_options)
    : mount_id(mount_id), parent_id(parent_id), device(device),
      root(root), mount_point(mount_point),
      optional_fields(optional_fields), filesystem_type(filesystem_type),
      mount_source(mount_source)
{
    this->mount_options = parseMountOptions(mount_options);
    this->super_options = parseMountOptions(super_options);
}

int mountinfo_entry::getMountId() const { return mount_id; }
int mountinfo_entry::getParentId() const { return parent_id; }
dev_t mountinfo_entry::getDevice() const { return device; }
const std::string &mountinfo_entry::getRoot() const { return root; }
const std::string &mountinfo_entry::getMountPoint() const { return mount_point; }
const std::unordered_map<std::string, std::string> &mountinfo_entry::getMountOptions() const { return mount_options; }
const std::string &mountinfo_entry::getOptionalFields() const { return optional_fields; }
const std::string &mountinfo_entry::getFilesystemType() const { return filesystem_type; }
const std::string &mountinfo_entry::getMountSource() const { return mount_source; }
const std::unordered_map<std::string, std::string> &mountinfo_entry::getSuperOptions() const { return super_options; }

const std::vector<mountinfo_entry> &Parsers::parseSelfMountinfo(bool cached)
{
    static std::vector<mountinfo_entry> parser_cache;
    if (cached && !parser_cache.empty())
    {
        return parser_cache;
    }
    parser_cache.clear();

    std::ifstream ifs("/proc/self/mountinfo", std::ifstream::in);
    if (!ifs)
    {
        LOGE("parseSelfMountinfo could not open /proc/self/mountinfo");
        return parser_cache;
    }

    for (std::string line; std::getline(ifs, line);)
    {
        std::istringstream iss(line);

        int mount_id, parent_id, _major, _minor;
        std::string root, mount_point, mount_options, optional_fields, filesystem_type, mount_source, super_options;
        char colon;

        // Read the first 6 fields (major, colon and minor are the same field)
        iss >> mount_id >> parent_id >> _major >> colon >> _minor >> root >> mount_point >> mount_options;
        if (iss.fail())
        {
            LOGE("parseSelfMountinfo failed to parse the first 6 fields of line: %s", line.c_str());
            continue;
        }

        std::string field;
        while (iss >> field && field != "-")
        {
            optional_fields += " " + field;
        }

        if (iss.fail())
        {
            LOGE("parseSelfMountinfo failed to parse the optional fields of line: %s", line.c_str());
            continue;
        }

        iss >> filesystem_type >> mount_source >> super_options;
        if (iss.fail())
        {
            LOGE("parseSelfMountinfo failed to parse the last 3 fields of line: %s", line.c_str());
            continue;
        }

        parser_cache.emplace_back(mountinfo_entry(mount_id, parent_id, makedev(_major, _minor),
                                                    root, mount_point, mount_options,
                                                    optional_fields, filesystem_type, mount_source,
                                                    super_options));
    }

    return parser_cache;
}

mountinfo_root_resolver::mountinfo_root_resolver(const std::vector<mountinfo_entry> &mount_infos)
{
    for (const auto &mount_info : mount_infos)
    {
        if (mount_info.getRoot() == "/")
        {
            device_mount_map[mount_info.getDevice()] = mount_info.getMountPoint();
        }
    }
}

std::string mountinfo_root_resolver::resolveRootOf(const mountinfo_entry &mount_info) const
{
    auto dev = mount_info.getDevice();
    if (device_mount_map.contains(dev))
    {
        const auto &mount_root = device_mount_map.at(dev);

        // If mount root is /, mount_info root will already be the true root
        if (mount_root != "/")
            return mount_root + mount_info.getRoot();
    }

    return mount_info.getRoot();
}
