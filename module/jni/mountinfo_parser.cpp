#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include <unordered_map>

#include "mountinfo_parser.hpp"
#include "mount_parser.hpp"
#include "logging.hpp"

using namespace Parsers;

mountinfo_entry_t::mountinfo_entry_t(int mount_id, int parent_id, int major, int minor,
                                     const std::string &root, const std::string &mount_point,
                                     const std::string &mount_options, const std::string &optional_fields,
                                     const std::string &filesystem_type, const std::string &mount_source,
                                     const std::string &super_options)
    : mount_id(mount_id), parent_id(parent_id), major(major), minor(minor),
      root(root), mount_point(mount_point),
      optional_fields(optional_fields), filesystem_type(filesystem_type),
      mount_source(mount_source)
{
    this->mount_options = parseMountOptions(mount_options);
    this->super_options = parseMountOptions(super_options);
}

int mountinfo_entry_t::getMountId() const { return mount_id; }
int mountinfo_entry_t::getParentId() const { return parent_id; }
int mountinfo_entry_t::getMajor() const { return major; }
int mountinfo_entry_t::getMinor() const { return minor; }
const std::string &mountinfo_entry_t::getRoot() const { return root; }
const std::string &mountinfo_entry_t::getMountPoint() const { return mount_point; }
const std::unordered_map<std::string, std::string> &mountinfo_entry_t::getMountOptions() const { return mount_options; }
const std::string &mountinfo_entry_t::getOptionalFields() const { return optional_fields; }
const std::string &mountinfo_entry_t::getFilesystemType() const { return filesystem_type; }
const std::string &mountinfo_entry_t::getMountSource() const { return mount_source; }
const std::unordered_map<std::string, std::string> &mountinfo_entry_t::getSuperOptions() const { return super_options; }

const std::vector<mountinfo_entry_t> &Parsers::parseSelfMountinfo(bool cached)
{
    static std::vector<mountinfo_entry_t> parser_cache;
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

        int mount_id, parent_id, major, minor;
        std::string root, mount_point, mount_options, optional_fields, filesystem_type, mount_source, super_options;
        char colon;

        // Read the first 6 fields (major, colon and minor are the same field)
        iss >> mount_id >> parent_id >> major >> colon >> minor >> root >> mount_point >> mount_options;
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

        parser_cache.emplace_back(mountinfo_entry_t(mount_id, parent_id, major, minor,
                                                    root, mount_point, mount_options,
                                                    optional_fields, filesystem_type, mount_source,
                                                    super_options));
    }

    return parser_cache;
}
