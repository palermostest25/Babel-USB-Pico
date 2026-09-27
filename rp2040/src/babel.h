// SPDX-License-Identifier: MIT
#ifndef BABEL_H_
#define BABEL_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define BABEL_NAME_LENGTH 2u
#define BABEL_ALPHABET_LENGTH 70u
#define BABEL_DIRECTORY_COUNT 4900u
#define BABEL_OBJECTS_PER_DIRECTORY (BABEL_DIRECTORY_COUNT + 1u)
#define BABEL_FILE_LOCAL_HANDLE BABEL_OBJECTS_PER_DIRECTORY
#define BABEL_MAX_FILE_SIZE 4096u
#define BABEL_MAX_DEPTH 2700u

typedef struct {
  uint8_t data[BABEL_MAX_FILE_SIZE];
  size_t length;
  uint16_t components[BABEL_MAX_DEPTH]; // one-based directory handles
  uint32_t depth;
} babel_state_t;

void babel_init(babel_state_t *state);
bool babel_select_directory(babel_state_t *state, uint32_t object_handle);
void babel_reset_to_root(babel_state_t *state);

bool babel_handle_valid(uint32_t object_handle);
bool babel_handle_is_file(uint32_t object_handle);
uint32_t babel_handle_depth(uint32_t object_handle);
uint32_t babel_local_handle(uint32_t object_handle);
uint32_t babel_child_handle(uint32_t depth, uint32_t local_handle);
uint32_t babel_parent_handle(const babel_state_t *state, uint32_t depth);

void babel_directory_name(uint32_t local_handle, uint16_t out[BABEL_NAME_LENGTH + 1u]);
const uint8_t *babel_file_data(const babel_state_t *state);
size_t babel_file_size(const babel_state_t *state);

#endif
