/*
 * Copyright (c) 2023, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <dirent.h>
#include <errno.h>
#include <pwd.h>
#include <sys/stat.h>
#include <unistd.h>

#include <fs.h>
#include <sys/syslimits.h>

DA_IMPL(DirEntry);

size_t fs_file_size(StringView file_name)
{
    struct stat st;
    if (stat(sv_cstr(file_name), &st) == 0) {
        fatal("fs_file_size(%.*s): Cannot stat '%.*s'", SV_ARG(file_name), SV_ARG(file_name));
    }
    return st.st_size;
}

bool fs_file_exists(StringView file_name)
{
    return access(sv_cstr(file_name), F_OK) == 0;
}

bool fs_is_directory(StringView file_name)
{
    struct stat st;
    if (stat(sv_cstr(file_name), &st) != 0) {
        fatal("fs_is_directory(%.*s): Cannot stat '%.*s'", SV_ARG(file_name), SV_ARG(file_name));
    }
    return (st.st_mode & S_IFDIR) != 0;
}

bool fs_is_symlink(StringView file_name)
{
    struct stat st;
    if (stat(sv_cstr(file_name), &st) != 0) {
        fatal("fs_is_symlink(%.*s): Cannot stat '%.*s'", SV_ARG(file_name), SV_ARG(file_name));
    }
    return S_ISLNK(st.st_mode);
}

bool fs_is_newer(StringView file_name1, StringView file_name2)
{
    struct stat st1, st2;
    if (stat(sv_cstr(file_name1), &st1) != 0) {
        fatal("fs_is_newer(%.*s, %.*s): Cannot stat '%.*s'", SV_ARG(file_name1), SV_ARG(file_name2), SV_ARG(file_name1));
    }
    if (stat(sv_cstr(file_name2), &st2) != 0) {
        fatal("fs_is_newer(%.*s, %.*s): Cannot stat '%.*s'", SV_ARG(file_name1), SV_ARG(file_name2), SV_ARG(file_name2));
    }
    if (st1.st_mtimespec.tv_sec == st2.st_mtimespec.tv_sec) {
        return st1.st_mtimespec.tv_nsec > st2.st_mtimespec.tv_nsec;
    }
    return st1.st_mtimespec.tv_sec > st2.st_mtimespec.tv_sec;
}

ErrorOrInt fs_assert_dir(StringView dir)
{
    if (fs_file_exists(dir)) {
        if (fs_is_directory(dir)) {
            RETURN(Int, 0);
        }
        ERROR(Int, IOError, 0, "'fs_assert_dir('%.*s'): Exists and is not a directory", SV_ARG(dir));
    }
    if (mkdir(sv_cstr(dir), 0700) != 0) {
        ERROR(Int, IOError, errno, "fs_assert_dir('%.*s'): Cannot create: %s", SV_ARG(dir), strerror(errno));
    }
    RETURN(Int, 0);
}

ErrorOrStringView fs_follow(StringView file_name)
{
    if (!fs_file_exists(file_name)) {
        ERROR(StringView, IOError, 0, "fs_follow('%.*s'): Does not exist", SV_ARG(file_name));
    }
    if (!fs_is_symlink(file_name)) {
        ERROR(StringView, IOError, 0, "fs_follow('%.*s'): Not a symlink", SV_ARG(file_name));
    }
    char followed[PATH_MAX + 1];
    memset(followed, '\0', PATH_MAX + 1);
    int len = readlink(sv_cstr(file_name), followed, PATH_MAX);
    if (len < 0) {
        ERROR(StringView, IOError, 0, "fs_follow('%.*s'): Error reading symlink: %s", SV_ARG(file_name), strerror(errno));
    }
    assert(len <= PATH_MAX)
        followed[len]
        = '\0';
    RETURN(StringView, sv_copy((StringView) { followed, len }));
}

ErrorOrInt fs_unlink(StringView file_name)
{
    if (unlink(sv_cstr(file_name)) < 0) {
        ERROR(Int, IOError, 0, "Error unlinking '%.*s': %s", SV_ARG(file_name), strerror(errno));
    }
    RETURN(Int, 0);
}

// @leak
StringView fs_canonical(StringView name)
{
    assert(sv_not_empty(name));
    char parent[PATH_MAX + 1];
    memset(parent, '\0', PATH_MAX + 1);

    if (name.ptr[0] == '~') {
        struct passwd *pw = getpwuid(getuid());
        strncpy(parent, pw->pw_dir, PATH_MAX);
    } else if (name.ptr[0] != '/') {
        getcwd(parent, PATH_MAX);
    }
    StringView parent_view = sv_from(parent);
    while (parent_view.length > 0 && parent_view.ptr[0] == '/') {
        ++parent_view.ptr;
        --parent_view.length;
    }
    while (parent_view.length > 0 && parent_view.ptr[parent_view.length - 1] == '/') {
        --parent_view.length;
    }
    StringList path = sv_split(parent_view, sv_from("/"));
    StringView name_stripped = name;
    while (name_stripped.length > 0 && name_stripped.ptr[0] == '/') {
        ++name_stripped.ptr;
        --name_stripped.length;
    }
    while (name_stripped.length > 0 && name_stripped.ptr[name_stripped.length - 1] == '/') {
        --name_stripped.length;
    }
    StringList name_components = sv_split(name_stripped, sv_from("/"));
    sl_extend(&path, &name_components);

    StringBuilder sb = { 0 };
    StringList    canonical_path = { 0 };
    sl_push(&canonical_path, sv_null());
    for (StringView comp = sl_pop_front(&path); !sv_empty(comp); comp = sl_pop_front(&path)) {
        if (sv_eq_cstr(comp, "..")) {
            if (canonical_path.size > 1) {
                sl_pop(&canonical_path);
                sb.view.length = 0;
                sb_append_list(&sb, &canonical_path, sv_from("/"));
            }
        } else if (!sv_eq_cstr(comp, ".")) {
            sl_push(&canonical_path, comp);
            sb_append_char(&sb, '/');
            sb_append_sv(&sb, comp);
            if (fs_file_exists(sb.view) && fs_is_symlink(sb.view)) {
                StringView followed = MUST(StringView, fs_follow(sb.view));
                assert(followed.length > 0);
                while (followed.ptr[0] == '/') {
                    ++followed.ptr;
                    --followed.length;
                    canonical_path = (StringList) { 0 };
                    sl_push(&canonical_path, sv_from(""));
                    sb.view.length = 0;
                    sb_append_char(&sb, '/');
                }
                while (followed.length > 0 && followed.ptr[followed.length - 1] == '/') {
                    --followed.length;
                }
                StringList followed_path = sv_split(followed, sv_from("/"));
                sl_pop(&canonical_path);
                sb.view.length = 0;
                sb_append_list(&sb, &canonical_path, sv_from("/"));
                StringList tail = path;
                path = (StringList) { 0 };
                sl_extend(&path, &followed_path);
                sl_extend(&path, &tail);
            }
        }
    }
    return sb.view;
}

// @leak
StringView fs_relative(StringView name, StringView base)
{
    StringList path = sv_split(fs_canonical(name), sv_from("/"));
    StringList base_path = sv_split(fs_canonical(base), sv_from("/"));

    if (path.size < base_path.size) {
        return name;
    }
    for (size_t ix = 0; ix < base_path.size; ++ix) {
        if (!sv_eq(base_path.strings[ix], path.strings[ix])) {
            return name;
        }
    }
    StringList tail = sl_split(&path, base_path.size);
    return sl_join(&tail, sv_from("/"));
}

ErrorOrDirListing fs_directory(StringView name, uint8_t options)
{
    char ch = name.ptr[name.length];
    ((char *) name.ptr)[name.length] = 0;
    DIR *dir = opendir(name.ptr);
    ((char *) name.ptr)[name.length] = ch;
    if (dir == NULL) {
        ERROR(DirListing, IOError, errno, "Could not open directory '%.*s': %s", SV_ARG(name), strerror(errno));
    }

    DirListing ret = { 0 };
    ret.directory = sv_copy(name);
    struct dirent *dp;
    while ((dp = readdir(dir)) != NULL) {
        FileType type = 0;
        if (dp->d_type == DT_DIR) {
            if (!(options & DirOptionDirectories)) {
                continue;
            }
            type = FileTypeDirectory;
        } else if (dp->d_type == DT_REG) {
            if (!(options & DirOptionFiles)) {
                continue;
            }
            type = FileTypeRegularFile;
        } else if (dp->d_type == DT_LNK) {
            if (!(options & DirOptionFiles)) {
                continue;
            }
            type = FileTypeSymlink;
        } else if (dp->d_type == DT_SOCK) {
            if (!(options & DirOptionFiles)) {
                continue;
            }
            type = FileTypeSocket;
        } else {
            continue;
        }
        if (dp->d_name[0] == '.' && type == FileTypeRegularFile && !(options & DirOptionHiddenFiles)) {
            continue;
        }

        StringView entry_name = { 0 };
#ifdef HAVE_DIRENT_D_NAMLEN
        entry_name = sv_copy((StringView) { dp->d_name, dp->d_namlen });
#else
        entry_name = sv_copy_cstr(dp->d_name);
#endif
        da_append_DirEntry(&ret.entries, (DirEntry) { .name = entry_name, .type = type });
    }
    closedir(dir);
    RETURN(DirListing, ret);
}

void dl_free(DirListing dir)
{
    for (size_t ix = 0; ix < dir.entries.size; ++ix) {
        DirEntry *entry = da_element_DirEntry(&dir.entries, ix);
        sv_free(entry->name);
    }
    da_free_DirEntry(&dir.entries);
    sv_free(dir.directory);
}
