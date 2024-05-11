#include "../libWad/Wad.h"
#include "../libWad/Wad.cpp"
#include <fuse.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <vector>

#define FUSE_USE_VERSION 26

static int fs_getattr(const char *path, struct stat *stbuf) {
    memset(stbuf, 0, sizeof(struct stat));
    if (((Wad*)fuse_get_context()->private_data)->isContent(path)) {
            stbuf->st_mode = S_IFREG | 0444;
            stbuf->st_nlink = 1;
            stbuf->st_size = ((Wad*)fuse_get_context()->private_data)->getSize(path);
    } else if (((Wad*)fuse_get_context()->private_data)->isDirectory(path)) {
        stbuf->st_mode = S_IFDIR | 0555;
        stbuf->st_nlink = 2;
    } else return -ENOENT;
    return 0;
}
static int fs_mknod(const char *path, mode_t mode, dev_t rdev) {
    if (!S_ISREG(mode) && !S_ISDIR(mode)) return -EACCES;
    if (S_ISREG(mode)) ((Wad*)fuse_get_context()->private_data)->createFile(path);
    else if (S_ISDIR(mode)) ((Wad*)fuse_get_context()->private_data)->createDirectory(path);
    return 0;
}
static int fs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info* fi) {
    return ((Wad*)fuse_get_context()->private_data)->getContents(path, buf, size, offset);
}
static int fs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info* fi) {
    return ((Wad*)fuse_get_context()->private_data)->writeToFile(path, buf, size, offset);
}
static int fs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
    std::vector<std::string> directoryEntries;
    ((Wad*)fuse_get_context()->private_data)->getDirectory(path, &directoryEntries);
    for (const auto& entry : directoryEntries) {filler(buf, entry.c_str(), nullptr, 0);}
    return 0;
}
int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cout<<"Not enough arguments." << std::endl;
        exit(EXIT_SUCCESS);
    }
    std::string wadPath = argv[argc-2];
    if(wadPath.at(0) != '/') wadPath = std::string(get_current_dir_name()) + "/" + wadPath;
    Wad* wad = Wad::loadWad(wadPath);
    argv[argc-2] = argv[argc-1];
    argc--;
    static struct fuse_operations fs_operations = {
        .getattr = fs_getattr,
        .mknod = fs_mknod,
        .read = fs_read,
        .write = fs_write,
        .readdir = fs_readdir,
    };
    return fuse_main(argc, argv, &fs_operations, wad);
}
