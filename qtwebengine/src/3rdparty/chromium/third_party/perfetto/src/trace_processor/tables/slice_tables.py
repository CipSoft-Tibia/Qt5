# Copyright (C) 2023 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
"""Contains tables for relevant for slices."""

from python.generators.trace_processor_table.public import Column as C
from python.generators.trace_processor_table.public import ColumnDoc
from python.generators.trace_processor_table.public import ColumnFlag
from python.generators.trace_processor_table.public import CppInt32
from python.generators.trace_processor_table.public import CppInt64
from python.generators.trace_processor_table.public import CppOptional
from python.generators.trace_processor_table.public import CppSelfTableId
from python.generators.trace_processor_table.public import CppString
from python.generators.trace_processor_table.public import CppTableId
from python.generators.trace_processor_table.public import CppUint32
from python.generators.trace_processor_table.public import Table
from python.generators.trace_processor_table.public import TableDoc
from python.generators.trace_processor_table.public import WrappingSqlView

from src.trace_processor.tables.track_tables import TRACK_TABLE

SLICE_TABLE = Table(
    python_module=__file__,
    class_name='SliceTable',
    sql_name='internal_slice',
    columns=[
        C('ts', CppInt64(), flags=ColumnFlag.SORTED),
        C('dur', CppInt64()),
        C('track_id', CppTableId(TRACK_TABLE)),
        C('category', CppOptional(CppString())),
        C('name', CppOptional(CppString())),
        C('depth', CppUint32()),
        C('stack_id', CppInt64()),
        C('parent_stack_id', CppInt64()),
        C('parent_id', CppOptional(CppSelfTableId())),
        C('arg_set_id', CppUint32()),
        C('thread_ts', CppOptional(CppInt64())),
        C('thread_dur', CppOptional(CppInt64())),
        C('thread_instruction_count', CppOptional(CppInt64())),
        C('thread_instruction_delta', CppOptional(CppInt64())),
    ],
    wrapping_sql_view=WrappingSqlView('slice'),
    tabledoc=TableDoc(
        doc='''
          Contains slices from userspace which explains what threads were doing
          during the trace.
        ''',
        group='Events',
        columns={
            'ts':
                'The timestamp at the start of the slice (in nanoseconds).',
            'dur':
                'The duration of the slice (in nanoseconds).',
            'track_id':
                'The id of the track this slice is located on.',
            'category':
                '''
                  The "category" of the slice. If this slice originated with
                  track_event, this column contains the category emitted.
                  Otherwise, it is likely to be null (with limited exceptions).
                ''',
            'name':
                '''
                  The name of the slice. The name describes what was happening
                  during the slice.
                ''',
            'depth':
                'The depth of the slice in the current stack of slices.',
            'stack_id':
                '''
                  A unique identifier obtained from the names of all slices
                  in this stack. This is rarely useful and kept around only
                  for legacy reasons.
                ''',
            'parent_stack_id':
                'The stack_id for the parent of this slice. Rarely useful.',
            'parent_id':
                '''
                  The id of the parent (i.e. immediate ancestor) slice for this
                  slice.
                ''',
            'arg_set_id':
                ColumnDoc(
                    'The id of the argument set associated with this slice.',
                    joinable='args.arg_set_id'),
            'thread_ts':
                '''
                  The thread timestamp at the start of the slice. This column
                  will only be populated if thread timestamp collection is
                  enabled with track_event.
                ''',
            'thread_dur':
                ''''
                  The thread time used by this slice. This column will only be
                  populated if thread timestamp collection is enabled with
                  track_event.
                ''',
            'thread_instruction_count':
                '''
                  The value of the CPU instruction counter at the start of the
                  slice. This column will only be populated if thread
                  instruction collection is enabled with track_event.
                ''',
            'thread_instruction_delta':
                '''
                  The change in value of the CPU instruction counter between the
                  start and end of the slice. This column will only be
                  populated if thread instruction collection is enabled with
                  track_event.
                ''',
        }))

GPU_SLICE_TABLE = Table(
    python_module=__file__,
    class_name='GpuSliceTable',
    sql_name='gpu_slice',
    columns=[
        C('context_id', CppOptional(CppInt64())),
        C('render_target', CppOptional(CppInt64())),
        C('render_target_name', CppString()),
        C('render_pass', CppOptional(CppInt64())),
        C('render_pass_name', CppString()),
        C('command_buffer', CppOptional(CppInt64())),
        C('command_buffer_name', CppString()),
        C('frame_id', CppOptional(CppUint32())),
        C('submission_id', CppOptional(CppUint32())),
        C('hw_queue_id', CppOptional(CppInt64())),
        C('render_subpasses', CppString()),
    ],
    parent=SLICE_TABLE,
    tabledoc=TableDoc(
        doc='''''',
        group='Events',
        columns={
            'context_id': '''''',
            'render_target': '''''',
            'render_target_name': '''''',
            'render_pass': '''''',
            'render_pass_name': '''''',
            'command_buffer': '''''',
            'command_buffer_name': '''''',
            'frame_id': '''''',
            'submission_id': '''''',
            'hw_queue_id': '''''',
            'render_subpasses': ''''''
        }))

GRAPHICS_FRAME_SLICE_TABLE = Table(
    python_module=__file__,
    class_name='GraphicsFrameSliceTable',
    sql_name='frame_slice',
    columns=[
        C('frame_number', CppUint32()),
        C('layer_name', CppString()),
        C('queue_to_acquire_time', CppInt64()),
        C('acquire_to_latch_time', CppInt64()),
        C('latch_to_present_time', CppInt64()),
    ],
    parent=SLICE_TABLE,
    tabledoc=TableDoc(
        doc='''''',
        group='Events',
        columns={
            'frame_number': '''''',
            'layer_name': '''''',
            'queue_to_acquire_time': '''''',
            'acquire_to_latch_time': '''''',
            'latch_to_present_time': ''''''
        }))

EXPECTED_FRAME_TIMELINE_SLICE_TABLE = Table(
    python_module=__file__,
    class_name='ExpectedFrameTimelineSliceTable',
    sql_name='expected_frame_timeline_slice',
    columns=[
        C('display_frame_token', CppInt64()),
        C('surface_frame_token', CppInt64()),
        C('upid', CppUint32()),
        C('layer_name', CppString()),
    ],
    parent=SLICE_TABLE,
    tabledoc=TableDoc(
        doc='''''',
        group='Events',
        columns={
            'display_frame_token': '''''',
            'surface_frame_token': '''''',
            'upid': '''''',
            'layer_name': ''''''
        }))

ACTUAL_FRAME_TIMELINE_SLICE_TABLE = Table(
    python_module=__file__,
    class_name='ActualFrameTimelineSliceTable',
    sql_name='actual_frame_timeline_slice',
    columns=[
        C('display_frame_token', CppInt64()),
        C('surface_frame_token', CppInt64()),
        C('upid', CppUint32()),
        C('layer_name', CppString()),
        C('present_type', CppString()),
        C('on_time_finish', CppInt32()),
        C('gpu_composition', CppInt32()),
        C('jank_type', CppString()),
        C('prediction_type', CppString()),
        C('jank_tag', CppString()),
    ],
    parent=SLICE_TABLE,
    tabledoc=TableDoc(
        doc='''''',
        group='Events',
        columns={
            'display_frame_token': '''''',
            'surface_frame_token': '''''',
            'upid': '''''',
            'layer_name': '''''',
            'present_type': '''''',
            'on_time_finish': '''''',
            'gpu_composition': '''''',
            'jank_type': '''''',
            'prediction_type': '''''',
            'jank_tag': ''''''
        }))

EXPERIMENTAL_FLAT_SLICE_TABLE = Table(
    python_module=__file__,
    class_name='ExperimentalFlatSliceTable',
    sql_name='experimental_flat_slice',
    columns=[
        C('ts', CppInt64()),
        C('dur', CppInt64()),
        C('track_id', CppTableId(TRACK_TABLE)),
        C('category', CppOptional(CppString())),
        C('name', CppOptional(CppString())),
        C('arg_set_id', CppUint32()),
        C('source_id', CppOptional(CppTableId(SLICE_TABLE))),
        C('start_bound', CppInt64(), flags=ColumnFlag.HIDDEN),
        C('end_bound', CppInt64(), flags=ColumnFlag.HIDDEN),
    ],
    tabledoc=TableDoc(
        doc='''
          An experimental table which "flattens" stacks of slices to contain
          only the "deepest" slice at any point in time on each track.
        ''',
        group='Events',
        columns={
            'ts':
                '''The timestamp at the start of the slice (in nanoseconds).''',
            'dur':
                '''The duration of the slice (in nanoseconds).''',
            'track_id':
                'The id of the track this slice is located on.',
            'category':
                '''
                  The "category" of the slice. If this slice originated with
                  track_event, this column contains the category emitted.
                  Otherwise, it is likely to be null (with limited exceptions).
                ''',
            'name':
                '''
                  The name of the slice. The name describes what was happening
                  during the slice.
                ''',
            'arg_set_id':
                ColumnDoc(
                    'The id of the argument set associated with this slice.',
                    joinable='args.arg_set_id'),
            'source_id':
                'The id of the slice which this row originated from.',
        }))

# Keep this list sorted.
ALL_TABLES = [
    ACTUAL_FRAME_TIMELINE_SLICE_TABLE,
    EXPECTED_FRAME_TIMELINE_SLICE_TABLE,
    EXPERIMENTAL_FLAT_SLICE_TABLE,
    GPU_SLICE_TABLE,
    GRAPHICS_FRAME_SLICE_TABLE,
    SLICE_TABLE,
]
