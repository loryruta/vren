#pragma once

#include <volk.h>

namespace vren
{
    /// A RAII wrapper to a generic Vulkan handle: takes care of destroying it, when the wrapper itself is deleted.
    template <typename HandleT> class HandleDeleter
    {
    public:
    private:
        HandleT m_handle;

    public:
        explicit HandleDeleter(HandleT handle) :
            m_handle(handle)
        {
        }

        HandleDeleter(HandleDeleter const& other) = delete;
        HandleDeleter(HandleDeleter&& other) noexcept :
            m_handle(other.m_handle)
        {
            other.m_handle = VK_NULL_HANDLE;
        }

        ~HandleDeleter()
        {
            if (m_handle != VK_NULL_HANDLE)
                delete_handle(m_handle);
        }

        HandleT get() const { return m_handle; }

    private:
        /// The function charged of deleting the handle, must be specialized.
        void delete_handle(HandleT handle) {}
    };
} // namespace vren
