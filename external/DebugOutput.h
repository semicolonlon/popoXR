// Copyright 2023, The Khronos Group Inc.
//
// SPDX-License-Identifier: Apache-2.0

// OpenXR Tutorial for Khronos Group

#pragma once
#ifdef _MSC_VER
#define NOMINMAX
#include <direct.h>
#include <windows.h>
#ifndef _MAX_PATH
#define _MAX_PATH 500
#endif
#include <time.h>

#include <cerrno>
#include <fstream>
#include <iostream>
#include <sstream>

#ifndef _MSC_VER
#define __stdcall
#endif
typedef void(__stdcall *DebugOutputCallback)(const char *);

class vsBufferedStringStreamBuf : public std::streambuf {
public:
    vsBufferedStringStreamBuf(int bufferSize) {
        if (bufferSize) {
            char *ptr = new char[bufferSize];
            setp(ptr, ptr + bufferSize);
        } else
            setp(0, 0);
    }
    virtual ~vsBufferedStringStreamBuf() {
        // sync();
        delete[] pbase();
    }
    virtual void writeString(const std::string &str) = 0;

private:
    int overflow(int c) {
        sync();

        if (c != EOF) {
            if (pbase() == epptr()) {
                std::string temp;
                temp += char(c);
                writeString(temp);
            } else
                sputc((char)c);
        }

        return 0;
    }

    int sync() {
        if (pbase() != pptr()) {
            int len = int(pptr() - pbase());
            std::string temp(pbase(), len);
            writeString(temp);
            setp(pbase(), epptr());
        }
        return 0;
    }
};

class DebugOutput : public vsBufferedStringStreamBuf {
public:
    DebugOutput(size_t bufsize = (size_t)16)
        : vsBufferedStringStreamBuf((int)bufsize), old_cout_buffer(NULL), old_cerr_buffer(NULL) {
        old_cout_buffer = std::cout.rdbuf(this);
        old_cerr_buffer = std::cerr.rdbuf(this);
    }
    virtual ~DebugOutput() {
        std::cout.rdbuf(old_cout_buffer);
        std::cerr.rdbuf(old_cerr_buffer);
    }
    virtual void writeString(const std::string &str) {
        OutputDebugStringA(str.c_str());
    }

protected:
    std::streambuf *old_cout_buffer;
    std::streambuf *old_cerr_buffer;
};

#define XR_LOG(...) std::cout << __VA_ARGS__ << "\n"
#define XR_LOG_ERROR(...) std::cerr << __VA_ARGS__ << "\n"
#endif
