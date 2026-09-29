// SPDX-License-Identifier: MIT
// MTP callback structure follows TinyUSB's MIT-licensed MTP device example.
#include <limits.h>
#include <string.h>

#include "babel.h"
#include "pico/unique_id.h"
#include "tusb.h"

#define DEVICE_MANUFACTURER "p2r3 / RP2040 port"
#define DEVICE_MODEL "USB of Babel"
#define DEVICE_VERSION "1.0.1-rp2040"
#define DEVICE_FRIENDLY_NAME "USB of Babel"
#define FIXED_DATETIME "20260608T000000.0"

#define STORAGE_DESCRIPTION {'d','i','s','k',0}
#define VOLUME_IDENTIFIER {'b','a','b','e','l',0}

enum { STORAGE_ID = 0x00010001u };

typedef MTP_STORAGE_INFO_STRUCT(
  TU_ARRAY_SIZE((uint16_t[])STORAGE_DESCRIPTION),
  TU_ARRAY_SIZE((uint16_t[])VOLUME_IDENTIFIER)
) storage_info_t;

static storage_info_t storage_info = {
  .storage_type = MTP_STORAGE_TYPE_FIXED_ROM,
  .filesystem_type = MTP_FILESYSTEM_TYPE_GENERIC_HIERARCHICAL,
  .access_capability = MTP_ACCESS_CAPABILITY_READ_ONLY_WITHOUT_OBJECT_DELETION,
  .max_capacity_in_bytes = UINT64_MAX / 2u,
  .free_space_in_bytes = 0,
  .free_space_in_objects = 0,
  .storage_description = {
    .count = (TU_FIELD_SIZE(storage_info_t, storage_description) - 1u) / sizeof(uint16_t),
    .utf16 = STORAGE_DESCRIPTION,
  },
  .volume_identifier = {
    .count = (TU_FIELD_SIZE(storage_info_t, volume_identifier) - 1u) / sizeof(uint16_t),
    .utf16 = VOLUME_IDENTIFIER,
  },
};

static babel_state_t babel;
static bool session_open;
static uint16_t file_name[] = {'f','i','l','e',0};

typedef int32_t (*operation_handler_t)(tud_mtp_cb_data_t *cb);
static int32_t get_device_info(tud_mtp_cb_data_t *cb);
static int32_t open_close_session(tud_mtp_cb_data_t *cb);
static int32_t get_storage_ids(tud_mtp_cb_data_t *cb);
static int32_t get_storage_info(tud_mtp_cb_data_t *cb);
static int32_t get_device_property(tud_mtp_cb_data_t *cb);
static int32_t get_object_handles(tud_mtp_cb_data_t *cb);
static int32_t get_object_info(tud_mtp_cb_data_t *cb);
static int32_t get_object_prop_value(tud_mtp_cb_data_t *cb);
static int32_t get_object(tud_mtp_cb_data_t *cb);
static int32_t get_partial_object(tud_mtp_cb_data_t *cb);

typedef struct { uint16_t code; operation_handler_t handler; } operation_t;
static const operation_t operations[] = {
  {MTP_OP_GET_DEVICE_INFO, get_device_info},
  {MTP_OP_OPEN_SESSION, open_close_session},
  {MTP_OP_CLOSE_SESSION, open_close_session},
  {MTP_OP_GET_STORAGE_IDS, get_storage_ids},
  {MTP_OP_GET_STORAGE_INFO, get_storage_info},
  {MTP_OP_GET_DEVICE_PROP_DESC, get_device_property},
  {MTP_OP_GET_DEVICE_PROP_VALUE, get_device_property},
  {MTP_OP_GET_OBJECT_HANDLES, get_object_handles},
  {MTP_OP_GET_OBJECT_INFO, get_object_info},
  {MTP_OP_GET_OBJECT_PROP_VALUE, get_object_prop_value},
  {MTP_OP_GET_OBJECT, get_object},
  {MTP_OP_GET_PARTIAL_OBJECT, get_partial_object},
};

static operation_handler_t find_handler(uint16_t code) {
  for (size_t i = 0; i < TU_ARRAY_SIZE(operations); ++i) {
    if (operations[i].code == code) return operations[i].handler;
  }
  return NULL;
}

bool tud_mtp_request_cancel_cb(tud_mtp_request_cb_data_t *cb) {
  (void)cb;
  return true;
}

bool tud_mtp_request_device_reset_cb(tud_mtp_request_cb_data_t *cb) {
  (void)cb;
  session_open = false;
  babel_reset_to_root(&babel);
  return true;
}

int32_t tud_mtp_request_get_extended_event_cb(tud_mtp_request_cb_data_t *cb) {
  (void)cb;
  return -1;
}

int32_t tud_mtp_request_get_device_status_cb(tud_mtp_request_cb_data_t *cb) {
  uint16_t *status = (uint16_t *)(uintptr_t)cb->buf;
  status[0] = 4;
  status[1] = MTP_RESP_OK;
  return 4;
}

static int32_t dispatch(tud_mtp_cb_data_t *cb) {
  operation_handler_t handler = find_handler(cb->command_container->header.code);
  int32_t response = handler ? handler(cb) : MTP_RESP_OPERATION_NOT_SUPPORTED;
  if (response > MTP_RESP_UNDEFINED) {
    cb->io_container.header->code = (uint16_t)response;
    tud_mtp_response_send(&cb->io_container);
  }
  return response;
}

int32_t tud_mtp_command_received_cb(tud_mtp_cb_data_t *cb) { return dispatch(cb); }

int32_t tud_mtp_data_xfer_cb(tud_mtp_cb_data_t *cb) {
  (void)dispatch(cb);
  return 0;
}

int32_t tud_mtp_data_complete_cb(tud_mtp_cb_data_t *cb) {
  mtp_container_info_t *response = &cb->io_container;
  if (cb->command_container->header.code == MTP_OP_GET_PARTIAL_OBJECT) {
    uint32_t length = cb->total_xferred_bytes - sizeof(mtp_container_header_t);
    (void)mtp_container_add_uint32(response, length);
  }
  response->header->code = cb->xfer_result == XFER_RESULT_SUCCESS
                            ? MTP_RESP_OK : MTP_RESP_GENERAL_ERROR;
  tud_mtp_response_send(response);
  return 0;
}

int32_t tud_mtp_response_complete_cb(tud_mtp_cb_data_t *cb) {
  (void)cb;
  return 0;
}

static size_t board_serial(uint16_t *out, size_t capacity) {
  pico_unique_board_id_t id;
  pico_get_unique_board_id(&id);
  static const char hex[] = "0123456789ABCDEF";
  size_t count = PICO_UNIQUE_BOARD_ID_SIZE_BYTES * 2u;
  if (count > capacity) count = capacity;
  for (size_t i = 0; i < count; ++i) {
    uint8_t byte = id.id[i / 2u];
    out[i] = (uint16_t)hex[(i & 1u) ? (byte & 0x0fu) : (byte >> 4)];
  }
  return count;
}

static int32_t send_or_busy(mtp_container_info_t *container) {
  return tud_mtp_data_send(container) ? 0 : MTP_RESP_DEVICE_BUSY;
}

static int32_t get_device_info(tud_mtp_cb_data_t *cb) {
  mtp_container_info_t *io = &cb->io_container;
  (void)mtp_container_add_cstring(io, DEVICE_MANUFACTURER);
  (void)mtp_container_add_cstring(io, DEVICE_MODEL);
  (void)mtp_container_add_cstring(io, DEVICE_VERSION);
  uint16_t serial[33];
  size_t count = board_serial(serial, 32);
  serial[count] = 0;
  (void)mtp_container_add_string(io, serial);
  return send_or_busy(io);
}

static int32_t open_close_session(tud_mtp_cb_data_t *cb) {
  if (cb->command_container->header.code == MTP_OP_OPEN_SESSION) {
    if (session_open) return MTP_RESP_SESSION_ALREADY_OPEN;
    session_open = true;
    babel_init(&babel);
  } else {
    if (!session_open) return MTP_RESP_SESSION_NOT_OPEN;
    session_open = false;
    babel_reset_to_root(&babel);
  }
  return MTP_RESP_OK;
}

static int32_t get_storage_ids(tud_mtp_cb_data_t *cb) {
  uint32_t ids[] = {STORAGE_ID};
  (void)mtp_container_add_auint32(&cb->io_container, 1, ids);
  return send_or_busy(&cb->io_container);
}

static int32_t get_storage_info(tud_mtp_cb_data_t *cb) {
  if (cb->command_container->params[0] != STORAGE_ID) return MTP_RESP_INVALID_STORAGE_ID;
  (void)mtp_container_add_raw(&cb->io_container, &storage_info, sizeof(storage_info));
  return send_or_busy(&cb->io_container);
}

static int32_t get_device_property(tud_mtp_cb_data_t *cb) {
  mtp_container_info_t *io = &cb->io_container;
  uint16_t property = (uint16_t)cb->command_container->params[0];
  if (property != MTP_DEV_PROP_DEVICE_FRIENDLY_NAME) return MTP_RESP_PARAMETER_NOT_SUPPORTED;

  if (cb->command_container->header.code == MTP_OP_GET_DEVICE_PROP_DESC) {
    mtp_device_prop_desc_header_t descriptor = {
      .device_property_code = property,
      .datatype = MTP_DATA_TYPE_STR,
      .get_set = MTP_MODE_GET,
    };
    (void)mtp_container_add_raw(io, &descriptor, sizeof(descriptor));
    (void)mtp_container_add_cstring(io, DEVICE_FRIENDLY_NAME);
    (void)mtp_container_add_cstring(io, DEVICE_FRIENDLY_NAME);
    (void)mtp_container_add_uint8(io, 0);
  } else {
    (void)mtp_container_add_cstring(io, DEVICE_FRIENDLY_NAME);
  }
  return send_or_busy(io);
}

static int32_t get_object_handles(tud_mtp_cb_data_t *cb) {
  const mtp_container_command_t *command = cb->command_container;
  mtp_container_info_t *io = &cb->io_container;
  uint32_t storage = command->params[0];
  uint32_t parent = command->params[2];
  if (storage != UINT32_MAX && storage != STORAGE_ID) return MTP_RESP_INVALID_STORAGE_ID;

  uint32_t depth;
  if (cb->phase == MTP_PHASE_COMMAND) {
    if (parent == UINT32_MAX || parent == 0) {
      babel_reset_to_root(&babel);
    } else if (!babel_select_directory(&babel, parent)) {
      return MTP_RESP_INVALID_PARENT_OBJECT;
    }
    depth = babel.depth;

    uint32_t first_payload_bytes = io->payload_bytes;
    uint32_t *write = (uint32_t *)io->payload;
    *write++ = BABEL_OBJECTS_PER_DIRECTORY;
    io->header->len += sizeof(uint32_t);

    uint32_t first_count = tu_min32(BABEL_OBJECTS_PER_DIRECTORY,
                                    (first_payload_bytes - sizeof(uint32_t)) / sizeof(uint32_t));
    for (uint32_t i = 0; i < first_count; ++i) {
      *write++ = babel_child_handle(depth, i + 1u);
    }
    io->header->len += BABEL_OBJECTS_PER_DIRECTORY * sizeof(uint32_t);
    return send_or_busy(io);
  }

  depth = babel.depth;
  uint32_t data_sent = cb->total_xferred_bytes - sizeof(mtp_container_header_t);
  uint32_t handles_sent = (data_sent - sizeof(uint32_t)) / sizeof(uint32_t);
  uint32_t remaining = BABEL_OBJECTS_PER_DIRECTORY - handles_sent;
  uint32_t count = tu_min32(remaining, io->payload_bytes / sizeof(uint32_t));
  uint32_t *write = (uint32_t *)io->payload;
  for (uint32_t i = 0; i < count; ++i) {
    write[i] = babel_child_handle(depth, handles_sent + i + 1u);
  }
  return send_or_busy(io);
}

static bool object_matches_path(uint32_t handle) {
  return babel_handle_valid(handle) && babel_handle_depth(handle) <= babel.depth;
}

static int32_t get_object_info(tud_mtp_cb_data_t *cb) {
  uint32_t handle = cb->command_container->params[0];
  if (!object_matches_path(handle)) return MTP_RESP_INVALID_OBJECT_HANDLE;

  uint32_t depth = babel_handle_depth(handle);
  bool is_file = babel_handle_is_file(handle);
  uint16_t directory_name[BABEL_NAME_LENGTH + 1u];
  uint16_t *name = file_name;
  if (!is_file) {
    babel_directory_name(babel_local_handle(handle), directory_name);
    name = directory_name;
  }

  mtp_object_info_header_t info = {
    .storage_id = STORAGE_ID,
    .object_format = is_file ? MTP_OBJ_FORMAT_TEXT : MTP_OBJ_FORMAT_ASSOCIATION,
    .protection_status = MTP_PROTECTION_STATUS_READ_ONLY,
    .object_compressed_size = is_file ? (uint32_t)babel_file_size(&babel) : 0,
    .thumb_format = MTP_OBJ_FORMAT_UNDEFINED,
    .parent_object = babel_parent_handle(&babel, depth),
    .association_type = is_file ? MTP_ASSOCIATION_UNDEFINED : MTP_ASSOCIATION_GENERIC_FOLDER,
  };
  mtp_container_info_t *io = &cb->io_container;
  (void)mtp_container_add_raw(io, &info, sizeof(info));
  (void)mtp_container_add_string(io, name);
  (void)mtp_container_add_cstring(io, FIXED_DATETIME);
  (void)mtp_container_add_cstring(io, FIXED_DATETIME);
  (void)mtp_container_add_cstring(io, "");
  return send_or_busy(io);
}

static int32_t get_object_prop_value(tud_mtp_cb_data_t *cb) {
  uint32_t handle = cb->command_container->params[0];
  uint16_t property = (uint16_t)cb->command_container->params[1];
  if (!object_matches_path(handle)) return MTP_RESP_INVALID_OBJECT_HANDLE;
  if (property != MTP_OBJ_PROP_OBJECT_FILE_NAME) {
    return MTP_RESP_OBJECT_PROP_NOT_SUPPORTED;
  }

  uint16_t directory_name[BABEL_NAME_LENGTH + 1u];
  uint16_t *name = file_name;
  if (!babel_handle_is_file(handle)) {
    babel_directory_name(babel_local_handle(handle), directory_name);
    name = directory_name;
  }
  (void)mtp_container_add_string(&cb->io_container, name);
  return send_or_busy(&cb->io_container);
}

static int32_t send_file_range(tud_mtp_cb_data_t *cb, uint32_t requested_offset,
                               uint32_t requested_length) {
  uint32_t handle = cb->command_container->params[0];
  if (!object_matches_path(handle) || !babel_handle_is_file(handle) ||
      babel_handle_depth(handle) != babel.depth) return MTP_RESP_INVALID_OBJECT_HANDLE;

  const uint8_t *data = babel_file_data(&babel);
  uint32_t size = (uint32_t)babel_file_size(&babel);
  uint32_t available = requested_offset >= size ? 0 : size - requested_offset;
  uint32_t total = tu_min32(available, requested_length);
  mtp_container_info_t *io = &cb->io_container;

  if (cb->phase == MTP_PHASE_COMMAND) {
    uint32_t safe_offset = requested_offset > size ? size : requested_offset;
    (void)mtp_container_add_raw(io, data + safe_offset, total);
    return send_or_busy(io);
  }
  if (cb->phase == MTP_PHASE_DATA) {
    uint32_t sent = cb->total_xferred_bytes - sizeof(mtp_container_header_t);
    uint32_t count = tu_min32(total - sent, io->payload_bytes);
    if (count != 0) {
      memcpy(io->payload, data + requested_offset + sent, count);
      return send_or_busy(io);
    }
  }
  return 0;
}

static int32_t get_object(tud_mtp_cb_data_t *cb) {
  return send_file_range(cb, 0, UINT32_MAX);
}

static int32_t get_partial_object(tud_mtp_cb_data_t *cb) {
  return send_file_range(cb, cb->command_container->params[1],
                         cb->command_container->params[2]);
}
