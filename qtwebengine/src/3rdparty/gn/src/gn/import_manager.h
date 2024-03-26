// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TOOLS_GN_IMPORT_MANAGER_H_
#define TOOLS_GN_IMPORT_MANAGER_H_

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_set>
#include <vector>

class Err;
class ParseNode;
class Scope;
class SourceFile;

// Provides a cache of the results of importing scopes so the results can
// be re-used rather than running the imported files multiple times.
class ImportManager {
 public:
  ImportManager();
  ~ImportManager();

  // Does an import of the given file into the given scope. On error, sets the
  // error and returns false.
  bool DoImport(const SourceFile& file,
                const ParseNode* node_for_err,
                Scope* scope,
                Err* err);

  std::vector<SourceFile> GetImportedFiles() const;

 private:
  struct ImportInfo;

  // Protects access to imports_ and imports_in_progress_. Do not hold when
  // actually executing imports.
  std::mutex imports_lock_;

  // Owning pointers to the scopes.
  using ImportMap = std::map<SourceFile, std::unique_ptr<ImportInfo>>;
  ImportMap imports_;

  std::unordered_set<std::string> imports_in_progress_;

  ImportManager(const ImportManager&) = delete;
  ImportManager& operator=(const ImportManager&) = delete;
};

#endif  // TOOLS_GN_IMPORT_MANAGER_H_
