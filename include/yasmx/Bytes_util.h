#ifndef YASM_BYTES_UTIL_H
#define YASM_BYTES_UTIL_H
///
/// @file
/// @brief Bytes utility functions interface.
///
/// @license
///  Copyright (C) 2008  Peter Johnson
///
/// Redistribution and use in source and binary forms, with or without
/// modification, are permitted provided that the following conditions
/// are met:
///  - Redistributions of source code must retain the above copyright
///    notice, this list of conditions and the following disclaimer.
///  - Redistributions in binary form must reproduce the above copyright
///    notice, this list of conditions and the following disclaimer in the
///    documentation and/or other materials provided with the distribution.
///
/// THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND OTHER CONTRIBUTORS ``AS IS''
/// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
/// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
/// ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR OTHER CONTRIBUTORS BE
/// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
/// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
/// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
/// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
/// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
/// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
/// POSSIBILITY OF SUCH DAMAGE.
/// @endlicense
///
#include "yasmx/Config/export.h"

#include "yasmx/Bytes.h"


namespace llvm
{
class APFloat;
class APInt;
}

namespace yasm
{

class IntNum;

/// Write an intnum as an 8-bit value to a bytes buffer.
/// @param bytes    output bytes buffer
/// @param intn     intnum
/// @warning Intnum is silently truncated to fit into 8 bits.
YASM_LIB_EXPORT
void Write8(Bytes& bytes, const IntNum& intn);

/// Write an intnum as a 16-bit value to a bytes buffer.
/// @param bytes    output bytes buffer
/// @param intn     intnum
/// @warning Intnum is silently truncated to fit into 16 bits.
YASM_LIB_EXPORT
void Write16(Bytes& bytes, const IntNum& intn);

/// Write an intnum as a 32-bit value to a bytes buffer.
/// @param bytes    output bytes buffer
/// @param intn     intnum
/// @warning Intnum is silently truncated to fit into 32 bits.
YASM_LIB_EXPORT
void Write32(Bytes& bytes, const IntNum& intn);

/// Write an intnum as a 64-bit value to a bytes buffer.
/// @param bytes    output bytes buffer
/// @param intn     intnum
/// @warning Intnum is silently truncated to fit into 64 bits.
YASM_LIB_EXPORT
void Write64(Bytes& bytes, const IntNum& intn);

/// Write an intnum as a N-bit value to a bytes buffer.
/// @param bytes    output bytes buffer
/// @param intn     intnum
/// @param n        number of bits (must be multiple of 8)
/// @warning Intnum is silently truncated to fit into N bits.
YASM_LIB_EXPORT
void WriteN(Bytes& bytes, const IntNum& intn, int n);

/// Write an 8-bit value to a bytes buffer.
/// @param bytes    output bytes buffer
/// @param val      8-bit value
inline void
Write8(Bytes& bytes, unsigned char val)
{
    bytes.push_back(val & 0xFF);
}

/// Write a 16-bit value to a bytes buffer.
/// @param bytes    output bytes buffer
/// @param val      16-bit value
inline void
Write16(Bytes& bytes, unsigned short val)
{
    if (bytes.isBigEndian())
    {
        bytes.push_back(static_cast<unsigned char>((val >> 8) & 0xFF));
        bytes.push_back(static_cast<unsigned char>(val & 0xFF));
    }
    else
    {
        bytes.push_back(static_cast<unsigned char>(val & 0xFF));
        bytes.push_back(static_cast<unsigned char>((val >> 8) & 0xFF));
    }
}

/// Write a 32-bit value to a bytes buffer.
/// @param bytes    output bytes buffer
/// @param val      32-bit value
inline void
Write32(Bytes& bytes, unsigned long val)
{
    if (bytes.isBigEndian())
    {
        bytes.push_back(static_cast<unsigned char>((val >> 24) & 0xFF));
        bytes.push_back(static_cast<unsigned char>((val >> 16) & 0xFF));
        bytes.push_back(static_cast<unsigned char>((val >> 8) & 0xFF));
        bytes.push_back(static_cast<unsigned char>(val & 0xFF));
    }
    else
    {
        bytes.push_back(static_cast<unsigned char>(val & 0xFF));
        bytes.push_back(static_cast<unsigned char>((val >> 8) & 0xFF));
        bytes.push_back(static_cast<unsigned char>((val >> 16) & 0xFF));
        bytes.push_back(static_cast<unsigned char>((val >> 24) & 0xFF));
    }
}

/// Write a N-bit value to a bytes buffer.
/// @param bytes    output bytes buffer
/// @param val      integer value
/// @param n        number of bits (must be multiple of 8)
/// @warning Value is silently truncated to fit into N bits.
YASM_LIB_EXPORT
void WriteN(Bytes& bytes, unsigned long val, int n);

/// Read an unsigned 8-bit value from a bytes buffer.
/// @param bytes    input bytes buffer
/// @return 8-bit value.
inline unsigned char
ReadU8(Bytes& bytes)
{
    const unsigned char* ptr = bytes.Read(1);
    return ptr[0] & 0xFF;
}

/// Read an signed 8-bit value from a bytes buffer.
/// @param bytes    input bytes buffer
/// @return 8-bit value.
inline signed char
ReadS8(Bytes& bytes)
{
    unsigned char val = ReadU8(bytes);
    if (val & 0x80)
        return -static_cast<signed char>((~val)+1);
    else
        return static_cast<signed char>(val);
}

/// Read an unsigned 16-bit value from a bytes buffer.
/// @param bytes    input bytes buffer
/// @return 16-bit value.
inline unsigned short
ReadU16(Bytes& bytes)
{
    const unsigned char* ptr = bytes.Read(2);
    unsigned short val = 0;
    if (bytes.isBigEndian())
    {
        val |= ptr[0] & 0xFF;
        val <<= 8;
        val |= ptr[1] & 0xFF;
    }
    else
    {
        val |= ptr[1] & 0xFF;
        val <<= 8;
        val |= ptr[0] & 0xFF;
    }
    return val;
}

/// Read an signed 16-bit value from a bytes buffer.
/// @param bytes    input bytes buffer
/// @return 16-bit value.
inline short
ReadS16(Bytes& bytes)
{
    unsigned short val = ReadU16(bytes);
    if (val & 0x8000)
        return -static_cast<short>((~val)+1);
    else
        return static_cast<short>(val);
}

/// Read an unsigned 32-bit value from a bytes buffer.
/// @param bytes    input bytes buffer
/// @return 32-bit value.
inline unsigned long
ReadU32(Bytes& bytes)
{
    const unsigned char* ptr = bytes.Read(4);
    unsigned long val = 0;
    if (bytes.isBigEndian())
    {
        val |= ptr[0] & 0xFF;
        val <<= 8;
        val |= ptr[1] & 0xFF;
        val <<= 8;
        val |= ptr[2] & 0xFF;
        val <<= 8;
        val |= ptr[3] & 0xFF;
    }
    else
    {
        val |= ptr[3] & 0xFF;
        val <<= 8;
        val |= ptr[2] & 0xFF;
        val <<= 8;
        val |= ptr[1] & 0xFF;
        val <<= 8;
        val |= ptr[0] & 0xFF;
    }
    return val;
}

/// Read an signed 32-bit value from a bytes buffer.
/// @param bytes    input bytes buffer
/// @return 32-bit value.
inline long
ReadS32(Bytes& bytes)
{
    unsigned long val = ReadU32(bytes);
    if (val & 0x80000000UL)
        return -static_cast<long>((~val)+1);
    else
        return static_cast<long>(val);
}

/// Read an unsigned N-bit value from a bytes buffer.
/// @param bytes    input bytes buffer
/// @param n        number of bits (must be multiple of 8)
/// @return N-bit value (as an IntNum).
YASM_LIB_EXPORT
IntNum ReadUnsigned(Bytes& bytes, int n);

/// Read an signed N-bit value from a bytes buffer.
/// @param bytes    input bytes buffer
/// @param n        number of bits (must be multiple of 8)
/// @return N-bit value (as an IntNum).
YASM_LIB_EXPORT
IntNum ReadSigned(Bytes& bytes, int n);

/// Read an unsigned 64-bit value from a bytes buffer.
/// @param bytes    input bytes buffer
/// @return 64-bit value (as an IntNum).
YASM_LIB_EXPORT
IntNum ReadU64(Bytes& bytes);

/// Read an signed 64-bit value from a bytes buffer.
/// @param bytes    input bytes buffer
/// @return 64-bit value (as an IntNum).
YASM_LIB_EXPORT
IntNum ReadS64(Bytes& bytes);

/// Output APInt to bytes in little-endian or big-endian.
/// Puts the value into the least significant bits of the destination,
/// or may be shifted into more significant bits by the shift parameter.
/// The destination bits are cleared before being set.
/// [0] should be the first byte output to the file.
/// @param bytes        output bytes buffer
/// @param intn         APInt
/// @param size         size (in bits)
/// @param shift        left shift (in bits); may be negative to specify
///                     right shift (standard warnings include truncation
///                     to boundary)
/// @param bigendian    endianness (true=big, false=little)
/// @param warn         enables standard warnings (value doesn't fit into
///                     valsize bits): <0=signed warnings,
///                     >0=unsigned warnings, 0=no warn
YASM_LIB_EXPORT
void Overwrite(Bytes& bytes,
               const llvm::APInt& intn,
               unsigned int size,
               int shift,
               bool bigendian,
               int warn);

/// Output intnum to bytes in little-endian or big-endian.
/// Puts the value into the least significant bits of the destination,
/// or may be shifted into more significant bits by the shift parameter.
/// The destination bits are cleared before being set.
/// [0] should be the first byte output to the file.
/// @param bytes        output bytes buffer
/// @param intn         intnum
/// @param size         size (in bits)
/// @param shift        left shift (in bits); may be negative to specify
///                     right shift (standard warnings include truncation
///                     to boundary)
/// @param bigendian    endianness (true=big, false=little)
/// @param warn         enables standard warnings (value doesn't fit into
///                     valsize bits): <0=signed warnings,
///                     >0=unsigned warnings, 0=no warn
YASM_LIB_EXPORT
void Overwrite(Bytes& bytes,
               const IntNum& intn,
               unsigned int size,
               int shift,
               bool bigendian,
               int warn);

YASM_LIB_EXPORT
void Overwrite(Bytes& bytes,
               const llvm::APFloat& flt,
               unsigned int size,
               int shift,
               bool bigendian,
               int warn);

} // namespace yasm

#endif
