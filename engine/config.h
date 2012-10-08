#ifndef __CONFIG_H__
#define __CONFIG_H__

// Here they go all the definitions

#define INDEXER_MAX_LOG_MSG 1024
#define MAX_FILENAME 255

#define LEVEL_INFO    0
#define LEVEL_DEBUG   1
#define LEVEL_WARNING 2
#define LEVEL_ERROR   3

#define RESTART_INTERVAL 100
#define SKIPLIST_SIZE 1024 * 1024 * 100// Actually this is not the memory occupation
#define MAX_SKIPLIST_ALLOCATION 1024 * 1024 * 100

#define POOL_SIZE 1024 * 8
#define BLOCK_SIZE 1024 * 1024 // Assuming 1MB block
#define START_MAP_SIZE 1024

#define START_DIRECTORY "/tmp"

#define MAGIC_STR "pedobear"
#define IS_LITTLE_ENDIAN 1
#define FOOTER_SIZE 40

#define MAX_LEVELS 7
#define MAX_FILES_LEVEL0 2
#define MAX_FILES 100

#define EXPANSION_LIMIT 15 * 2 * 1048576
#define GRANDPARENT_OVERLAP 10 * 2 * 1048576
#define MAX_MEM_COMPACT_LEVEL 2

#endif