wadfs: wadfs.cpp ../libWad
	g++ -D_FILE_OFFSET_BITS=64 -DFUSE_USE_VERSION=26 wadfs.cpp -o wadfs -lfuse -L ../libWad -lWad -w

# 				./wadfs/wadfs -s DOOM1.WAD ./mountdir


# 				./wadfs/wadfs -s sample1.wad ./mountdir


# 				sudo chmod 666 /dev/fuse


# 				fusermount -u mountdir