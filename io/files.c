/*
this files includes posix file functions
*/

#include <io/include/files.h>
#include <include/types.h>
#include <mm/include/obj_alloc.h>
#include <mm/include/kmalloc.h>
#include <include/mem.h>

bool files_ready = false;
pool_t files_descr_pool;
uint32_t files_pool_last_ind;
file_t stdin, stdout, stderr;

bool init_files() {
    if (!create_obj_pool(&files_descr_pool, sizeof(file_t), MAX_FILES, manual)) {
        return false;
    }

    //create default files (stdin, stdout, stderr)
    obj_pool_put(files_descr_pool, (void *) &stdin, 0);
    obj_pool_put(files_descr_pool, (void *) &stdout, 1);
    obj_pool_put(files_descr_pool, (void *) &stderr, 2);

    files_pool_last_ind = 3;
    files_ready = true;
    return true;
}

void init_file_descriptor(file_t *file, file_type_t type) {
    file->type = type;
    file->size = 0;
    file->ptr = null;
}

file_t *get_file(uint32_t fd) {
    if (!files_ready || fd >= MAX_FILES) {
        return null;
    }

    file_t *file;
    if (!obj_pool_get(files_descr_pool, (void **) &file, fd)) {
        return null;
    }

    return file;
}

uint64_t write(uint32_t fd, void *buffer, uint64_t count) {
    if (!buffer || count == 0 || !files_ready || fd >= MAX_FILES) {
        return 0;
    }

    file_t *file = get_file(fd);
    if (!file) {
        return false;
    }

    uint32_t new_size = file->size + count;

    if (file->ptr) {
        if (!(file->ptr = krealloc(file->ptr, new_size))) {
            return 0;
        }
    } else {
        if (!(file->ptr = kmalloc(new_size))) {
            return 0;
        }
    }

    memcpy(file->ptr + file->size, buffer, count);
    file->size += count;
    return count;
}

uint64_t read(uint32_t fd, void *buffer, uint64_t count) {
    if (count == 0 || fd >= MAX_FILES || !files_ready) {
        return 0;
    }

    file_t *file = get_file(fd);

    if (!file) {
        return 0;
    }

    if (count > file->size) {
        count = file->size;
    }

    if (buffer) {
        memcpy(buffer, file->ptr, count);
    }

    file->size -= count;
    return count;
}