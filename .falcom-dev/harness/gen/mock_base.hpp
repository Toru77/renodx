#pragma once
#include <include/reshade.hpp>
namespace mock {
using namespace reshade::api;
struct DeviceBase : reshade::api::device {
  uint64_t get_native() const override { return {}; }
  void get_private_data(const uint8_t guid[16], uint64_t *data) const override {}
  void set_private_data(const uint8_t guid[16], const uint64_t data) override {}
  device_api get_api() const override { return {}; }
  bool check_capability(device_caps capability) const override { return {}; }
  bool check_format_support(format format, resource_usage usage) const override { return {}; }
  bool create_sampler(const sampler_desc &desc, sampler *out_sampler) override { return {}; }
  void destroy_sampler(sampler sampler) override {}
  bool create_resource(const resource_desc &desc, const subresource_data *initial_data, resource_usage initial_state, resource *out_resource, void **shared_handle = nullptr) override { return {}; }
  void destroy_resource(resource resource) override {}
  resource_desc get_resource_desc(resource resource) const override { return {}; }
  bool create_resource_view(resource resource, resource_usage usage_type, const resource_view_desc &desc, resource_view *out_view) override { return {}; }
  void destroy_resource_view(resource_view view) override {}
  resource get_resource_from_view(resource_view view) const override { return {}; }
  resource_view_desc get_resource_view_desc(resource_view view) const override { return {}; }
  bool map_buffer_region(resource resource, uint64_t offset, uint64_t size, map_access access, void **out_data) override { return {}; }
  void unmap_buffer_region(resource resource) override {}
  bool map_texture_region(resource resource, uint32_t subresource, const subresource_box *box, map_access access, subresource_data *out_data) override { return {}; }
  void unmap_texture_region(resource resource, uint32_t subresource) override {}
  void update_buffer_region(const void *data, resource dest, uint64_t dest_offset, uint64_t size) override {}
  void update_texture_region(const subresource_data &data, resource dest, uint32_t dest_subresource, const subresource_box *dest_box = nullptr) override {}
  bool create_pipeline(pipeline_layout layout, uint32_t subobject_count, const pipeline_subobject *subobjects, pipeline *out_pipeline) override { return {}; }
  void destroy_pipeline(pipeline pipeline) override {}
  bool create_pipeline_layout(uint32_t param_count, const pipeline_layout_param *params, pipeline_layout *out_layout) override { return {}; }
  void destroy_pipeline_layout(pipeline_layout layout) override {}
  bool allocate_descriptor_tables(uint32_t count, pipeline_layout layout, uint32_t param, descriptor_table *out_tables) override { return {}; }
  void free_descriptor_tables(uint32_t count, const descriptor_table *tables) override {}
  void get_descriptor_heap_offset(descriptor_table table, uint32_t binding, uint32_t array_offset, descriptor_heap *out_heap, uint32_t *out_offset) const override {}
  void copy_descriptor_tables(uint32_t count, const descriptor_table_copy *copies) override {}
  void update_descriptor_tables(uint32_t count, const descriptor_table_update *updates) override {}
  bool create_query_heap(query_type type, uint32_t count, query_heap *out_heap) override { return {}; }
  void destroy_query_heap(query_heap heap) override {}
  bool get_query_heap_results(query_heap heap, uint32_t first, uint32_t count, void *results, uint32_t stride) override { return {}; }
  void set_resource_name(resource resource, const char *name) override {}
  void set_resource_view_name(resource_view view, const char *name) override {}
  bool create_fence(uint64_t initial_value, fence_flags flags, fence *out_fence, void **shared_handle = nullptr) override { return {}; }
  void destroy_fence(fence fence) override {}
  uint64_t get_completed_fence_value(fence fence) const override { return {}; }
  bool wait(fence fence, uint64_t value, uint64_t timeout = UINT64_MAX) override { return {}; }
  bool signal(fence fence, uint64_t value) override { return {}; }
  bool get_property(device_properties property, void *data) const override { return {}; }
  uint64_t get_resource_view_gpu_address(resource_view view) const override { return {}; }
  void get_acceleration_structure_size(acceleration_structure_type type, acceleration_structure_build_flags flags, uint32_t input_count, const acceleration_structure_build_input *inputs, uint64_t *out_size, uint64_t *out_build_scratch_size, uint64_t *out_update_scratch_size) const override {}
  bool get_pipeline_shader_group_handles(pipeline pipeline, uint32_t first, uint32_t count, void *out_handles) override { return {}; }
};
struct CommandListBase : reshade::api::command_list {
  uint64_t get_native() const override { return {}; }
  void get_private_data(const uint8_t guid[16], uint64_t *data) const override {}
  void set_private_data(const uint8_t guid[16], const uint64_t data) override {}
  device *get_device() override { return {}; }
  void barrier(uint32_t count, const resource *resources, const resource_usage *old_states, const resource_usage *new_states) override {}
  void begin_render_pass(uint32_t count, const render_pass_render_target_desc *rts, const render_pass_depth_stencil_desc *ds = nullptr) override {}
  void end_render_pass() override {}
  void bind_render_targets_and_depth_stencil(uint32_t count, const resource_view *rtvs, resource_view dsv = { 0 }) override {}
  void bind_pipeline(pipeline_stage stages, pipeline pipeline) override {}
  void bind_pipeline_states(uint32_t count, const dynamic_state *states, const uint32_t *values) override {}
  void bind_viewports(uint32_t first, uint32_t count, const viewport *viewports) override {}
  void bind_scissor_rects(uint32_t first, uint32_t count, const rect *rects) override {}
  void push_constants(shader_stage stages, pipeline_layout layout, uint32_t param, uint32_t first, uint32_t count, const void *values) override {}
  void push_descriptors(shader_stage stages, pipeline_layout layout, uint32_t param, const descriptor_table_update &update) override {}
  void bind_descriptor_tables(shader_stage stages, pipeline_layout layout, uint32_t first, uint32_t count, const descriptor_table *tables) override {}
  void bind_index_buffer(resource buffer, uint64_t offset, uint32_t index_size) override {}
  void bind_vertex_buffers(uint32_t first, uint32_t count, const resource *buffers, const uint64_t *offsets, const uint32_t *strides) override {}
  void bind_stream_output_buffers(uint32_t first, uint32_t count, const resource *buffers, const uint64_t *offsets, const uint64_t *max_sizes, const resource *counter_buffers, const uint64_t *counter_offsets) override {}
  void draw(uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex, uint32_t first_instance) override {}
  void draw_indexed(uint32_t index_count, uint32_t instance_count, uint32_t first_index, int32_t vertex_offset, uint32_t first_instance) override {}
  void dispatch(uint32_t group_count_x, uint32_t group_count_y, uint32_t group_count_z) override {}
  void draw_or_dispatch_indirect(indirect_command type, resource buffer, uint64_t offset, uint32_t draw_count, uint32_t stride) override {}
  void copy_resource(resource source, resource dest) override {}
  void copy_buffer_region(resource source, uint64_t source_offset, resource dest, uint64_t dest_offset, uint64_t size) override {}
  void copy_buffer_to_texture(resource source, uint64_t source_offset, uint32_t row_length, uint32_t slice_height, resource dest, uint32_t dest_subresource, const subresource_box *dest_box = nullptr) override {}
  void copy_texture_region(resource source, uint32_t source_subresource, const subresource_box *source_box, resource dest, uint32_t dest_subresource, const subresource_box *dest_box, filter_mode filter = filter_mode::min_mag_mip_point) override {}
  void copy_texture_to_buffer(resource source, uint32_t source_subresource, const subresource_box *source_box, resource dest, uint64_t dest_offset, uint32_t row_length = 0, uint32_t slice_height = 0) override {}
  void resolve_texture_region(resource source, uint32_t source_subresource, const subresource_box *source_box, resource dest, uint32_t dest_subresource, uint32_t dest_x, uint32_t dest_y, uint32_t dest_z, format format) override {}
  void clear_depth_stencil_view(resource_view dsv, const float *depth, const uint8_t *stencil, uint32_t rect_count = 0, const rect *rects = nullptr) override {}
  void clear_render_target_view(resource_view rtv, const float color[4], uint32_t rect_count = 0, const rect *rects = nullptr) override {}
  void clear_unordered_access_view_uint(resource_view uav, const uint32_t values[4], uint32_t rect_count = 0, const rect *rects = nullptr) override {}
  void clear_unordered_access_view_float(resource_view uav, const float values[4], uint32_t rect_count = 0, const rect *rects = nullptr) override {}
  void generate_mipmaps(resource_view srv) override {}
  void begin_query(query_heap heap, query_type type, uint32_t index) override {}
  void end_query(query_heap heap, query_type type, uint32_t index) override {}
  void copy_query_heap_results(query_heap heap, query_type type, uint32_t first, uint32_t count, resource dest, uint64_t dest_offset, uint32_t stride) override {}
  void begin_debug_event(const char *label, const float color[4] = nullptr) override {}
  void end_debug_event() override {}
  void insert_debug_marker(const char *label, const float color[4] = nullptr) override {}
  void dispatch_mesh(uint32_t group_count_x, uint32_t group_count_y, uint32_t group_count_z) override {}
  void dispatch_rays(resource raygen, uint64_t raygen_offset, uint64_t raygen_size, resource miss, uint64_t miss_offset, uint64_t miss_size, uint64_t miss_stride, resource hit_group, uint64_t hit_group_offset, uint64_t hit_group_size, uint64_t hit_group_stride, resource callable, uint64_t callable_offset, uint64_t callable_size, uint64_t callable_stride, uint32_t width, uint32_t height, uint32_t depth) override {}
  void copy_acceleration_structure(resource_view source, resource_view dest, acceleration_structure_copy_mode mode) override {}
  void build_acceleration_structure(acceleration_structure_type type, acceleration_structure_build_flags flags, uint32_t input_count, const acceleration_structure_build_input *inputs, resource scratch, uint64_t scratch_offset, resource_view source, resource_view dest, acceleration_structure_build_mode mode) override {}
  void query_acceleration_structures(uint32_t count, const resource_view *acceleration_structures, query_heap heap, query_type type, uint32_t first) override {}
  void update_buffer_region(const void *data, resource dest, uint64_t dest_offset, uint64_t size) override {}
  void update_texture_region(const subresource_data &data, resource dest, uint32_t dest_subresource, const subresource_box *dest_box = nullptr) override {}
};
struct QueueBase : reshade::api::command_queue {
  uint64_t get_native() const override { return {}; }
  void get_private_data(const uint8_t guid[16], uint64_t *data) const override {}
  void set_private_data(const uint8_t guid[16], const uint64_t data) override {}
  device *get_device() override { return {}; }
  command_queue_type get_type() const override { return {}; }
  void wait_idle() const override {}
  void flush_immediate_command_list() const override {}
  command_list *get_immediate_command_list() override { return {}; }
  void begin_debug_event(const char *label, const float color[4] = nullptr) override {}
  void end_debug_event() override {}
  void insert_debug_marker(const char *label, const float color[4] = nullptr) override {}
  bool wait(fence fence, uint64_t value) override { return {}; }
  bool signal(fence fence, uint64_t value) override { return {}; }
  uint64_t get_timestamp_frequency() const override { return {}; }
};
}
