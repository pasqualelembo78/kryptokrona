// Copyright (c) 2012-2017, The MevaCoin developers, The Bytecoin developers
// Copyright (c) 2014-2018, The Monero Project
// Copyright (c) 2018-2019, The TurtleCoin Developers
// Copyright (c) 2019, The Kryptokrona Developers
//
// Please see the included LICENSE file for more information.

#include "memory_mapped_file.h"

#include <cassert>

#define NOMINMAX
#include <windows.h>

#include "common/scope_exit.h"

namespace syst
{

    MemoryMappedFile::MemoryMappedFile() : m_fileHandle(INVALID_HANDLE_VALUE),
                                           m_mappingHandle(INVALID_HANDLE_VALUE),
                                           m_size(0),
                                           m_data(nullptr)
    {
    }

    MemoryMappedFile::~MemoryMappedFile()
    {
        std::error_code ignore;
        close(ignore);
    }

    const std::string &MemoryMappedFile::path() const
    {
        assert(isOpened());

        return m_path;
    }

    uint64_t MemoryMappedFile::size() const
    {
        assert(isOpened());

        return m_size;
    }

    const uint8_t *MemoryMappedFile::data() const
    {
        assert(isOpened());

        return m_data;
    }

    uint8_t *MemoryMappedFile::data()
    {
        assert(isOpened());

        return m_data;
    }

    bool MemoryMappedFile::isOpened() const
    {
        return m_data != nullptr;
    }

    void MemoryMappedFile::create(const std::string &path, uint64_t size, bool overwrite, std::error_code &ec)
    {
        if (isOpened())
        {
            close(ec);
            if (ec)
            {
                return;
            }
        }

        tools::ScopeExit failExitHandler([this, &ec]
                                         {
    ec = std::error_code(::GetLastError(), std::system_category());
    std::error_code ignore;
    close(ignore); });

        m_fileHandle = ::CreateFile(
            path.c_str(),
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_DELETE | FILE_SHARE_READ,
            NULL,
            overwrite ? CREATE_ALWAYS : CREATE_NEW,
            FILE_ATTRIBUTE_NORMAL,
            NULL);
        if (m_fileHandle == INVALID_HANDLE_VALUE)
        {
            return;
        }

        LONG distanceToMoveHigh = static_cast<LONG>((size >> 32) & UINT64_C(0xffffffff));
        DWORD filePointer = ::SetFilePointer(m_fileHandle, static_cast<LONG>(size & UINT64_C(0xffffffff)), &distanceToMoveHigh, FILE_BEGIN);
        if (filePointer == INVALID_SET_FILE_POINTER)
        {
            return;
        }

        BOOL result = ::SetEndOfFile(m_fileHandle);
        if (!result)
        {
            return;
        }

        m_mappingHandle = ::CreateFileMapping(m_fileHandle, NULL, PAGE_READWRITE, 0, 0, NULL);
        if (m_mappingHandle == NULL)
        {
            return;
        }

        m_data = reinterpret_cast<uint8_t *>(::MapViewOfFile(m_mappingHandle, FILE_MAP_ALL_ACCESS, 0, 0, 0));
        if (m_data == NULL)
        {
            return;
        }

        m_size = size;
        m_path = path;
        ec = std::error_code();

        failExitHandler.cancel();
    }

    void MemoryMappedFile::create(const std::string &path, uint64_t size, bool overwrite)
    {
        std::error_code ec;
        create(path, size, overwrite, ec);
        if (ec)
        {
            throw std::system_error(ec, "MemoryMappedFile::create");
        }
    }

    void MemoryMappedFile::open(const std::string &path, std::error_code &ec)
    {
        if (isOpened())
        {
            close(ec);
            if (ec)
            {
                return;
            }
        }

        tools::ScopeExit failExitHandler([this, &ec]
                                         {
    ec = std::error_code(::GetLastError(), std::system_category());
    std::error_code ignore;
    close(ignore); });

        m_fileHandle = ::CreateFile(
            path.c_str(),
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_DELETE | FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL);
        if (m_fileHandle == INVALID_HANDLE_VALUE)
        {
            return;
        }

        LARGE_INTEGER fileSize;
        BOOL result = ::GetFileSizeEx(m_fileHandle, &fileSize);
        if (!result)
        {
            return;
        }

        m_size = static_cast<uint64_t>(fileSize.QuadPart);

        m_mappingHandle = ::CreateFileMapping(m_fileHandle, NULL, PAGE_READWRITE, 0, 0, NULL);
        if (m_mappingHandle == NULL)
        {
            return;
        }

        m_data = reinterpret_cast<uint8_t *>(::MapViewOfFile(m_mappingHandle, FILE_MAP_ALL_ACCESS, 0, 0, 0));
        if (m_data == NULL)
        {
            return;
        }

        m_path = path;
        ec = std::error_code();

        failExitHandler.cancel();
    }

    void MemoryMappedFile::open(const std::string &path)
    {
        std::error_code ec;
        open(path, ec);
        if (ec)
        {
            throw std::system_error(ec, "MemoryMappedFile::open");
        }
    }

    void MemoryMappedFile::rename(const std::string &newPath, std::error_code &ec)
    {
        assert(isOpened());

        BOOL result = ::MoveFileEx(m_path.c_str(), newPath.c_str(), MOVEFILE_REPLACE_EXISTING);
        if (result)
        {
            m_path = newPath;
            ec = std::error_code();
        }
        else
        {
            ec = std::error_code(::GetLastError(), std::system_category());
        }
    }

    void MemoryMappedFile::rename(const std::string &newPath)
    {
        assert(isOpened());

        std::error_code ec;
        rename(newPath, ec);
        if (ec)
        {
            throw std::system_error(ec, "MemoryMappedFile::rename");
        }
    }

    void MemoryMappedFile::close(std::error_code &ec)
    {
        BOOL result;
        if (m_data != nullptr)
        {
            flush(m_data, m_size, ec);
            if (ec)
            {
                return;
            }

            result = ::UnmapViewOfFile(m_data);
            if (result)
            {
                m_data = nullptr;
            }
            else
            {
                ec = std::error_code(::GetLastError(), std::system_category());
                return;
            }
        }

        if (m_mappingHandle != INVALID_HANDLE_VALUE)
        {
            result = ::CloseHandle(m_mappingHandle);
            if (result)
            {
                m_mappingHandle = INVALID_HANDLE_VALUE;
            }
            else
            {
                ec = std::error_code(::GetLastError(), std::system_category());
                return;
            }
        }

        if (m_fileHandle != INVALID_HANDLE_VALUE)
        {
            result = ::CloseHandle(m_fileHandle);
            if (result)
            {
                m_fileHandle = INVALID_HANDLE_VALUE;
                ec = std::error_code();
            }
            else
            {
                ec = std::error_code(::GetLastError(), std::system_category());
                return;
            }
        }

        ec = std::error_code();
    }

    void MemoryMappedFile::close()
    {
        std::error_code ec;
        close(ec);
        if (ec)
        {
            throw std::system_error(ec, "MemoryMappedFile::close");
        }
    }

    void MemoryMappedFile::flush(uint8_t *data, uint64_t size, std::error_code &ec)
    {
        assert(isOpened());

        BOOL result = ::FlushViewOfFile(data, static_cast<SIZE_T>(size));
        if (result)
        {
            result = ::FlushFileBuffers(m_fileHandle);
            if (result)
            {
                ec = std::error_code();
                return;
            }
        }

        ec = std::error_code(::GetLastError(), std::system_category());
    }

    void MemoryMappedFile::flush(uint8_t *data, uint64_t size)
    {
        assert(isOpened());

        std::error_code ec;
        flush(data, size, ec);
        if (ec)
        {
            throw std::system_error(ec, "MemoryMappedFile::flush");
        }
    }

    void MemoryMappedFile::swap(MemoryMappedFile &other)
    {
        std::swap(m_fileHandle, other.m_fileHandle);
        std::swap(m_mappingHandle, other.m_mappingHandle);
        std::swap(m_path, other.m_path);
        std::swap(m_data, other.m_data);
        std::swap(m_size, other.m_size);
    }

}
