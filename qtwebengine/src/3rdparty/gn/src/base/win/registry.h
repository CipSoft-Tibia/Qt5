// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_WIN_REGISTRY_H_
#define BASE_WIN_REGISTRY_H_

#include <stdint.h>
#include <windows.h>
#include <string>
#include <vector>

#include "base/win/scoped_handle.h"

namespace base {
namespace win {

// Utility class to read, write and manipulate the Windows Registry.
// Registry vocabulary primer: a "key" is like a folder, in which there
// are "values", which are <name, data> pairs, with an associated data type.
//
// Note:
//  * ReadValue family of functions guarantee that the out-parameter
//    is not touched in case of failure.
//  * Functions returning LONG indicate success as ERROR_SUCCESS or an
//    error as a (non-zero) win32 error code.
class RegKey {
 public:
  RegKey();
  explicit RegKey(HKEY key);
  RegKey(HKEY rootkey, const char16_t* subkey, REGSAM access);
  ~RegKey();

  LONG Create(HKEY rootkey, const char16_t* subkey, REGSAM access);

  LONG CreateWithDisposition(HKEY rootkey,
                             const char16_t* subkey,
                             DWORD* disposition,
                             REGSAM access);

  // Creates a subkey or open it if it already exists.
  LONG CreateKey(const char16_t* name, REGSAM access);

  // Opens an existing reg key.
  LONG Open(HKEY rootkey, const char16_t* subkey, REGSAM access);

  // Opens an existing reg key, given the relative key name.
  LONG OpenKey(const char16_t* relative_key_name, REGSAM access);

  // Closes this reg key.
  void Close();

  // Replaces the handle of the registry key and takes ownership of the handle.
  void Set(HKEY key);

  // Transfers ownership away from this object.
  HKEY Take();

  // Returns false if this key does not have the specified value, or if an error
  // occurs while attempting to access it.
  bool HasValue(const char16_t* value_name) const;

  // Returns the number of values for this key, or 0 if the number cannot be
  // determined.
  DWORD GetValueCount() const;

  // Determines the nth value's name.
  LONG GetValueNameAt(int index, std::u16string* name) const;

  // True while the key is valid.
  bool Valid() const { return key_ != NULL; }

  // Kills a key and everything that lives below it; please be careful when
  // using it.
  LONG DeleteKey(const char16_t* name);

  // Deletes an empty subkey.  If the subkey has subkeys or values then this
  // will fail.
  LONG DeleteEmptyKey(const char16_t* name);

  // Deletes a single value within the key.
  LONG DeleteValue(const char16_t* name);

  // Getters:

  // Reads a REG_DWORD (uint32_t) into |out_value|. If |name| is null or empty,
  // reads the key's default value, if any.
  LONG ReadValueDW(const char16_t* name, DWORD* out_value) const;

  // Reads a REG_QWORD (int64_t) into |out_value|. If |name| is null or empty,
  // reads the key's default value, if any.
  LONG ReadInt64(const char16_t* name, int64_t* out_value) const;

  // Reads a string into |out_value|. If |name| is null or empty, reads
  // the key's default value, if any.
  LONG ReadValue(const char16_t* name, std::u16string* out_value) const;

  // Reads a REG_MULTI_SZ registry field into a vector of strings. Clears
  // |values| initially and adds further strings to the list. Returns
  // ERROR_CANTREAD if type is not REG_MULTI_SZ.
  LONG ReadValues(const char16_t* name, std::vector<std::u16string>* values);

  // Reads raw data into |data|. If |name| is null or empty, reads the key's
  // default value, if any.
  LONG ReadValue(const char16_t* name,
                 void* data,
                 DWORD* dsize,
                 DWORD* dtype) const;

  // Setters:

  // Sets an int32_t value.
  LONG WriteValue(const char16_t* name, DWORD in_value);

  // Sets a string value.
  LONG WriteValue(const char16_t* name, const char16_t* in_value);

  // Sets raw data, including type.
  LONG WriteValue(const char16_t* name,
                  const void* data,
                  DWORD dsize,
                  DWORD dtype);

  HKEY Handle() const { return key_; }

 private:
  // Calls RegDeleteKeyEx on supported platforms, alternatively falls back to
  // RegDeleteKey.
  static LONG RegDeleteKeyExWrapper(HKEY hKey,
                                    const char16_t* lpSubKey,
                                    REGSAM samDesired,
                                    DWORD Reserved);

  // Recursively deletes a key and all of its subkeys.
  static LONG RegDelRecurse(HKEY root_key,
                            const std::u16string& name,
                            REGSAM access);

  HKEY key_;  // The registry key being iterated.
  REGSAM wow64access_;

  RegKey(const RegKey&) = delete;
  RegKey& operator=(const RegKey&) = delete;
};

// Iterates the entries found in a particular folder on the registry.
class RegistryValueIterator {
 public:
  // Constructs a Registry Value Iterator with default WOW64 access.
  RegistryValueIterator(HKEY root_key, const char16_t* folder_key);

  // Constructs a Registry Key Iterator with specific WOW64 access, one of
  // KEY_WOW64_32KEY or KEY_WOW64_64KEY, or 0.
  // Note: |wow64access| should be the same access used to open |root_key|
  // previously, or a predefined key (e.g. HKEY_LOCAL_MACHINE).
  // See http://msdn.microsoft.com/en-us/library/windows/desktop/aa384129.aspx.
  RegistryValueIterator(HKEY root_key,
                        const char16_t* folder_key,
                        REGSAM wow64access);

  ~RegistryValueIterator();

  DWORD ValueCount() const;

  // True while the iterator is valid.
  bool Valid() const;

  // Advances to the next registry entry.
  void operator++();

  const char16_t* Name() const { return name_.c_str(); }
  const char16_t* Value() const { return value_.data(); }
  // ValueSize() is in bytes.
  DWORD ValueSize() const { return value_size_; }
  DWORD Type() const { return type_; }

  int Index() const { return index_; }

 private:
  // Reads in the current values.
  bool Read();

  void Initialize(HKEY root_key,
                  const char16_t* folder_key,
                  REGSAM wow64access);

  // The registry key being iterated.
  HKEY key_;

  // Current index of the iteration.
  int index_;

  // Current values.
  std::u16string name_;
  std::vector<char16_t> value_;
  DWORD value_size_;
  DWORD type_;

  RegistryValueIterator(const RegistryValueIterator&) = delete;
  RegistryValueIterator& operator=(const RegistryValueIterator&) = delete;
};

class RegistryKeyIterator {
 public:
  // Constructs a Registry Key Iterator with default WOW64 access.
  RegistryKeyIterator(HKEY root_key, const char16_t* folder_key);

  // Constructs a Registry Value Iterator with specific WOW64 access, one of
  // KEY_WOW64_32KEY or KEY_WOW64_64KEY, or 0.
  // Note: |wow64access| should be the same access used to open |root_key|
  // previously, or a predefined key (e.g. HKEY_LOCAL_MACHINE).
  // See http://msdn.microsoft.com/en-us/library/windows/desktop/aa384129.aspx.
  RegistryKeyIterator(HKEY root_key,
                      const char16_t* folder_key,
                      REGSAM wow64access);

  ~RegistryKeyIterator();

  DWORD SubkeyCount() const;

  // True while the iterator is valid.
  bool Valid() const;

  // Advances to the next entry in the folder.
  void operator++();

  const char16_t* Name() const { return name_; }

  int Index() const { return index_; }

 private:
  // Reads in the current values.
  bool Read();

  void Initialize(HKEY root_key,
                  const char16_t* folder_key,
                  REGSAM wow64access);

  // The registry key being iterated.
  HKEY key_;

  // Current index of the iteration.
  int index_;

  char16_t name_[MAX_PATH];

  RegistryKeyIterator(const RegistryKeyIterator&) = delete;
  RegistryKeyIterator& operator=(const RegistryKeyIterator&) = delete;
};

}  // namespace win
}  // namespace base

#endif  // BASE_WIN_REGISTRY_H_
