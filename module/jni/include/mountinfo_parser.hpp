#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <sys/types.h>

namespace Parsers
{
    class mountinfo_entry
    {
    public:
        mountinfo_entry(int mount_id, int parent_id, dev_t device,
                          const std::string &root, const std::string &mount_point,
                          const std::string &mount_options, const std::string &optional_fields,
                          const std::string &filesystem_type, const std::string &mount_source,
                          const std::string &super_options);

        int getMountId() const;
        int getParentId() const;
        dev_t getDevice() const;
        const std::string &getRoot() const;
        const std::string &getMountPoint() const;
        const std::unordered_map<std::string, std::string> &getMountOptions() const;
        const std::string &getOptionalFields() const;
        const std::string &getFilesystemType() const;
        const std::string &getMountSource() const;
        const std::unordered_map<std::string, std::string> &getSuperOptions() const;

    private:
        dev_t device;
        int mount_id, parent_id;
        std::string root, mount_point, optional_fields, filesystem_type, mount_source;
        std::unordered_map<std::string, std::string> mount_options, super_options;
    };

    const std::vector<mountinfo_entry> &parseSelfMountinfo(bool cached = true);

    class mountinfo_root_resolver
    {
    public:
        mountinfo_root_resolver(const std::vector<mountinfo_entry> &mount_infos);
        std::string resolveRootOf(const mountinfo_entry &mount_info) const;

    private:
        std::unordered_map<dev_t, std::string> device_mount_map;
    };
}
