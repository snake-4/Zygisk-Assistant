#include "fd_reopener.hpp"
#include <fd_utils.h>
#include <string>
#include <utility>
#include "logging.hpp"

FDReopener::ScopedRegularReopener::ScopedRegularReopener()
{
    auto pFdSet = GetOpenFds([](const std::string &error)
                             { LOGE("GetOpenFds: %s", error.c_str()); });
    if (pFdSet)
    {
        for (const auto &fd : *pFdSet)
        {
            auto pFDI = FileDescriptorInfo::CreateFromFd(fd, [fd](const std::string &error)
                                                         { LOGE("CreateFromFd(%d): %s", fd, error.c_str()); });

            // Only process regular files that are not memfds
            if (pFDI && !pFDI->is_sock && !pFDI->file_path.starts_with("/memfd:"))
                fdi_vector.emplace_back(std::move(pFDI));
        }
    }
}

FDReopener::ScopedRegularReopener::~ScopedRegularReopener()
{
    for (const auto &pFDI : fdi_vector)
    {
        LOGD("Reopening FD %d with %s", pFDI->fd, pFDI->file_path.c_str());
        pFDI->ReopenOrDetach([fd = pFDI->fd](const std::string &error)
                             { LOGE("ReopenOrDetach(%d): %s", fd, error.c_str()); });
    }
}
