/*
 * Copyright (C) 2023 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef INCLUDE_PERFETTO_PUBLIC_PROTOS_TRACE_TRACK_EVENT_DEBUG_ANNOTATION_PZC_H_
#define INCLUDE_PERFETTO_PUBLIC_PROTOS_TRACE_TRACK_EVENT_DEBUG_ANNOTATION_PZC_H_

#include <stdbool.h>
#include <stdint.h>

#include "perfetto/public/pb_macros.h"

PERFETTO_PB_MSG_DECL(perfetto_protos_DebugAnnotation);
PERFETTO_PB_MSG_DECL(perfetto_protos_DebugAnnotation_NestedValue);

PERFETTO_PB_ENUM_IN_MSG(perfetto_protos_DebugAnnotation_NestedValue,
                        NestedType){
    PERFETTO_PB_ENUM_IN_MSG_ENTRY(perfetto_protos_DebugAnnotation_NestedValue,
                                  UNSPECIFIED) = 0,
    PERFETTO_PB_ENUM_IN_MSG_ENTRY(perfetto_protos_DebugAnnotation_NestedValue,
                                  DICT) = 1,
    PERFETTO_PB_ENUM_IN_MSG_ENTRY(perfetto_protos_DebugAnnotation_NestedValue,
                                  ARRAY) = 2,
};

PERFETTO_PB_MSG(perfetto_protos_DebugAnnotationValueTypeName);
PERFETTO_PB_FIELD(perfetto_protos_DebugAnnotationValueTypeName,
                  VARINT,
                  uint64_t,
                  iid,
                  1);
PERFETTO_PB_FIELD(perfetto_protos_DebugAnnotationValueTypeName,
                  STRING,
                  const char*,
                  name,
                  2);

PERFETTO_PB_MSG(perfetto_protos_DebugAnnotationName);
PERFETTO_PB_FIELD(perfetto_protos_DebugAnnotationName,
                  VARINT,
                  uint64_t,
                  iid,
                  1);
PERFETTO_PB_FIELD(perfetto_protos_DebugAnnotationName,
                  STRING,
                  const char*,
                  name,
                  2);

PERFETTO_PB_MSG(perfetto_protos_DebugAnnotation);
PERFETTO_PB_FIELD(perfetto_protos_DebugAnnotation,
                  VARINT,
                  uint64_t,
                  name_iid,
                  1);
PERFETTO_PB_FIELD(perfetto_protos_DebugAnnotation,
                  STRING,
                  const char*,
                  name,
                  10);
PERFETTO_PB_FIELD(perfetto_protos_DebugAnnotation, VARINT, bool, bool_value, 2);
PERFETTO_PB_FIELD(perfetto_protos_DebugAnnotation,
                  VARINT,
                  uint64_t,
                  uint_value,
                  3);
PERFETTO_PB_FIELD(perfetto_protos_DebugAnnotation,
                  VARINT,
                  int64_t,
                  int_value,
                  4);
PERFETTO_PB_FIELD(perfetto_protos_DebugAnnotation,
                  FIXED64,
                  double,
                  double_value,
                  5);
PERFETTO_PB_FIELD(perfetto_protos_DebugAnnotation,
                  VARINT,
                  uint64_t,
                  pointer_value,
                  7);
PERFETTO_PB_FIELD(perfetto_protos_DebugAnnotation,
                  MSG,
                  perfetto_protos_DebugAnnotation_NestedValue,
                  nested_value,
                  8);
PERFETTO_PB_FIELD(perfetto_protos_DebugAnnotation,
                  STRING,
                  const char*,
                  legacy_json_value,
                  9);
PERFETTO_PB_FIELD(perfetto_protos_DebugAnnotation,
                  STRING,
                  const char*,
                  string_value,
                  6);
PERFETTO_PB_FIELD(perfetto_protos_DebugAnnotation,
                  VARINT,
                  uint64_t,
                  string_value_iid,
                  17);
PERFETTO_PB_FIELD(perfetto_protos_DebugAnnotation,
                  STRING,
                  const char*,
                  proto_type_name,
                  16);
PERFETTO_PB_FIELD(perfetto_protos_DebugAnnotation,
                  VARINT,
                  uint64_t,
                  proto_type_name_iid,
                  13);
PERFETTO_PB_FIELD(perfetto_protos_DebugAnnotation,
                  STRING,
                  const char*,
                  proto_value,
                  14);
PERFETTO_PB_FIELD(perfetto_protos_DebugAnnotation,
                  MSG,
                  perfetto_protos_DebugAnnotation,
                  dict_entries,
                  11);
PERFETTO_PB_FIELD(perfetto_protos_DebugAnnotation,
                  MSG,
                  perfetto_protos_DebugAnnotation,
                  array_values,
                  12);

PERFETTO_PB_MSG(perfetto_protos_DebugAnnotation_NestedValue);
PERFETTO_PB_FIELD(perfetto_protos_DebugAnnotation_NestedValue,
                  VARINT,
                  enum perfetto_protos_DebugAnnotation_NestedValue_NestedType,
                  nested_type,
                  1);
PERFETTO_PB_FIELD(perfetto_protos_DebugAnnotation_NestedValue,
                  STRING,
                  const char*,
                  dict_keys,
                  2);
PERFETTO_PB_FIELD(perfetto_protos_DebugAnnotation_NestedValue,
                  MSG,
                  perfetto_protos_DebugAnnotation_NestedValue,
                  dict_values,
                  3);
PERFETTO_PB_FIELD(perfetto_protos_DebugAnnotation_NestedValue,
                  MSG,
                  perfetto_protos_DebugAnnotation_NestedValue,
                  array_values,
                  4);
PERFETTO_PB_FIELD(perfetto_protos_DebugAnnotation_NestedValue,
                  VARINT,
                  int64_t,
                  int_value,
                  5);
PERFETTO_PB_FIELD(perfetto_protos_DebugAnnotation_NestedValue,
                  FIXED64,
                  double,
                  double_value,
                  6);
PERFETTO_PB_FIELD(perfetto_protos_DebugAnnotation_NestedValue,
                  VARINT,
                  bool,
                  bool_value,
                  7);
PERFETTO_PB_FIELD(perfetto_protos_DebugAnnotation_NestedValue,
                  STRING,
                  const char*,
                  string_value,
                  8);

#endif  // INCLUDE_PERFETTO_PUBLIC_PROTOS_TRACE_TRACK_EVENT_DEBUG_ANNOTATION_PZC_H_
