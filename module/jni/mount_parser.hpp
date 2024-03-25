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
    void parseMountOptions(const std::string &input);

    std::string fsname;
    std::string dir;
    std::string type;
    std::unordered_map<std::string, std::string> opts_map;
    int freq;
    int passno;
};

std::vector<mount_entry_t> parseMountsFromPath(const char *path);
