/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __FS_H__
#define __FS_H__

#include <base/sv.h>

typedef enum : uint8_t {
    DirOptionFiles = 0x01,
    DirOptionDirectories = 0x02,
    DirOptionHiddenFiles = 0x04,
    DirOptionAll = 0x07,
} DirOption;

typedef enum {
    FileTypeDirectory = 0x01,
    FileTypeRegularFile = 0x02,
    FileTypeSocket = 0x04,
    FileTypeSymlink = 0x08,
} FileType;

typedef struct {
    StringView name;
    FileType   type;
} DirEntry;

DA_WITH_NAME(DirEntry, DirEntries);

typedef struct {
    StringView directory;
    DirEntries entries;
} DirListing;

ERROR_OR(DirListing);

extern size_t            fs_file_size(StringView file_name);
extern bool              fs_file_exists(StringView file_name);
extern bool              fs_is_directory(StringView file_name);
extern bool              fs_is_symlink(StringView file_name);
extern bool              fs_is_newer(StringView file_name1, StringView file_name2);
extern ErrorOrInt        fs_assert_dir(StringView dir);
extern ErrorOrStringView fs_follow(StringView file_name);
extern ErrorOrInt        fs_unlink(StringView file_name);
extern StringView        fs_canonical(StringView name);
extern StringView        fs_relative(StringView name, StringView base);
extern ErrorOrDirListing fs_directory(StringView name, uint8_t options);
extern void              dl_free(DirListing dir);

#endif /* __FS_H__ */
