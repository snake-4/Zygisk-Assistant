#pragma once
#include "fd_utils.h"
#include <vector>
#include <memory>

namespace FDReopener
{
    class ScopedRegularReopener
    {
    public:
        ScopedRegularReopener();
        ~ScopedRegularReopener();

    private:
        ScopedRegularReopener(const ScopedRegularReopener &);
        void operator=(const ScopedRegularReopener &);
        std::vector<std::unique_ptr<FileDescriptorInfo>> fdi_vector;
    };
}
