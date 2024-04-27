#pragma once
#include <string>
#include <vector>
#include <unordered_map>

class mountinfo_entry_t
{
public:
    mountinfo_entry_t(int mount_id, int parent_id, int major, int minor,
                      const std::string &root, const std::string &mount_point,
                      const std::string &mount_options, const std::string &optional_fields,
                      const std::string &filesystem_type, const std::string &mount_source,
                      const std::string &super_options);

    int getMountId() const;
    int getParentId() const;
    int getMajor() const;
    int getMinor() const;
    const std::string &getRoot() const;
    const std::string &getMountPoint() const;
    const std::unordered_map<std::string, std::string> &getMountOptions() const;
    const std::string &getOptionalFields() const;
    const std::string &getFilesystemType() const;
    const std::string &getMountSource() const;
    const std::unordered_map<std::string, std::string> &getSuperOptions() const;

private:
    int mount_id, parent_id, major, minor;
    std::string root, mount_point, optional_fields, filesystem_type, mount_source;
    std::unordered_map<std::string, std::string> mount_options, super_options;
};

const std::vector<mountinfo_entry_t> &parseSelfMountinfo(bool cached = true);
