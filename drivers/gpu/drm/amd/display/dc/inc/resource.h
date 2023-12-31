/*
 * Copyright 2015 Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors: AMD
 */

#ifndef DRIVERS_GPU_DRM_AMD_DC_DEV_DC_INC_RESOURCE_H_
#define DRIVERS_GPU_DRM_AMD_DC_DEV_DC_INC_RESOURCE_H_

#include "core_types.h"
#include "core_status.h"
#include "dal_asic_id.h"
#include "dm_pp_smu.h"

#define MEMORY_TYPE_MULTIPLIER_CZ 4
#define MEMORY_TYPE_HBM 2


#define IS_PIPE_SYNCD_VALID(pipe) ((((pipe)->pipe_idx_syncd) & 0x80)?1:0)
#define GET_PIPE_SYNCD_FROM_PIPE(pipe) ((pipe)->pipe_idx_syncd & 0x7F)
#define SET_PIPE_SYNCD_TO_PIPE(pipe, pipe_syncd) ((pipe)->pipe_idx_syncd = (0x80 | pipe_syncd))

enum dce_version resource_parse_asic_id(
		struct hw_asic_id asic_id);

struct resource_caps {
	int num_timing_generator;
	int num_opp;
	int num_video_plane;
	int num_audio;
	int num_stream_encoder;
	int num_pll;
	int num_dwb;
	int num_ddc;
	int num_vmid;
	int num_dsc;
	unsigned int num_dig_link_enc; // Total number of DIGs (digital encoders) in DIO (Display Input/Output).
	unsigned int num_usb4_dpia; // Total number of USB4 DPIA (DisplayPort Input Adapters).
	int num_hpo_dp_stream_encoder;
	int num_hpo_dp_link_encoder;
	int num_mpc_3dlut;
};

struct resource_straps {
	uint32_t hdmi_disable;
	uint32_t dc_pinstraps_audio;
	uint32_t audio_stream_number;
};

struct resource_create_funcs {
	void (*read_dce_straps)(
			struct dc_context *ctx, struct resource_straps *straps);

	struct audio *(*create_audio)(
			struct dc_context *ctx, unsigned int inst);

	struct stream_encoder *(*create_stream_encoder)(
			enum engine_id eng_id, struct dc_context *ctx);

	struct hpo_dp_stream_encoder *(*create_hpo_dp_stream_encoder)(
			enum engine_id eng_id, struct dc_context *ctx);

	struct hpo_dp_link_encoder *(*create_hpo_dp_link_encoder)(
			uint8_t inst,
			struct dc_context *ctx);

	struct dce_hwseq *(*create_hwseq)(
			struct dc_context *ctx);
};

bool resource_construct(
	unsigned int num_virtual_links,
	struct dc *dc,
	struct resource_pool *pool,
	const struct resource_create_funcs *create_funcs);

struct resource_pool *dc_create_resource_pool(struct dc  *dc,
					      const struct dc_init_data *init_data,
					      enum dce_version dc_version);

void dc_destroy_resource_pool(struct dc *dc);

enum dc_status resource_map_pool_resources(
		const struct dc *dc,
		struct dc_state *context,
		struct dc_stream_state *stream);

bool resource_build_scaling_params(struct pipe_ctx *pipe_ctx);

enum dc_status resource_build_scaling_params_for_context(
		const struct dc *dc,
		struct dc_state *context);

void resource_build_info_frame(struct pipe_ctx *pipe_ctx);

void resource_unreference_clock_source(
		struct resource_context *res_ctx,
		const struct resource_pool *pool,
		struct clock_source *clock_source);

void resource_reference_clock_source(
		struct resource_context *res_ctx,
		const struct resource_pool *pool,
		struct clock_source *clock_source);

int resource_get_clock_source_reference(
		struct resource_context *res_ctx,
		const struct resource_pool *pool,
		struct clock_source *clock_source);

bool resource_are_streams_timing_synchronizable(
		struct dc_stream_state *stream1,
		struct dc_stream_state *stream2);

bool resource_are_vblanks_synchronizable(
		struct dc_stream_state *stream1,
		struct dc_stream_state *stream2);

struct clock_source *resource_find_used_clk_src_for_sharing(
		struct resource_context *res_ctx,
		struct pipe_ctx *pipe_ctx);

struct clock_source *dc_resource_find_first_free_pll(
		struct resource_context *res_ctx,
		const struct resource_pool *pool);

struct pipe_ctx *resource_get_head_pipe_for_stream(
		struct resource_context *res_ctx,
		struct dc_stream_state *stream);

bool resource_attach_surfaces_to_context(
		struct dc_plane_state *const *plane_state,
		int surface_count,
		struct dc_stream_state *dc_stream,
		struct dc_state *context,
		const struct resource_pool *pool);

/*
 * pipe types are identified based on MUXes in DCN front end that are capable
 * of taking input from one DCN pipeline to another DCN pipeline. The name is
 * in a form of XXXX_YYYY, where XXXX is the DCN front end hardware block the
 * pipeline ends with and YYYY is the rendering role that the pipe is in.
 *
 * For instance OTG_MASTER is a pipe ending with OTG hardware block in its
 * pipeline and it is in a role of a master pipe for timing generation.
 *
 * For quick reference a diagram of each pipe type's areas of responsibility
 * for outputting timings on the screen is shown below:
 *
 *       Timing Active for Stream 0
 *        __________________________________________________
 *       |OTG master 0 (OPP head 0)|OPP head 2 (DPP pipe 2) |
 *       |             (DPP pipe 0)|                        |
 *       | Top Plane 0             |                        |
 *       |           ______________|____                    |
 *       |          |DPP pipe 1    |DPP |                   |
 *       |          |              |pipe|                   |
 *       |          |  Bottom      |3   |                   |
 *       |          |  Plane 1     |    |                   |
 *       |          |              |    |                   |
 *       |          |______________|____|                   |
 *       |                         |                        |
 *       |                         |                        |
 *       | ODM slice 0             | ODM slice 1            |
 *       |_________________________|________________________|
 *
 *       Timing Active for Stream 1
 *        __________________________________________________
 *       |OTG master 4 (OPP head 4)                         |
 *       |                                                  |
 *       |                                                  |
 *       |                                                  |
 *       |                                                  |
 *       |                                                  |
 *       |               Blank Pixel Data                   |
 *       |              (generated by DPG4)                 |
 *       |                                                  |
 *       |                                                  |
 *       |                                                  |
 *       |                                                  |
 *       |                                                  |
 *       |__________________________________________________|
 *
 *       Inter-pipe Relation
 *        __________________________________________________
 *       |PIPE IDX|   DPP PIPES   | OPP HEADS | OTG MASTER  |
 *       |        |  plane 0      | slice 0   |             |
 *       |   0    | -------------MPC---------ODM----------- |
 *       |        |  plane 1    | |         | |             |
 *       |   1    | ------------- |         | |             |
 *       |        |  plane 0      | slice 1 | |             |
 *       |   2    | -------------MPC--------- |             |
 *       |        |  plane 1    | |           |             |
 *       |   3    | ------------- |           |             |
 *       |        |               | blank     |             |
 *       |   4    |               | ----------------------- |
 *       |        |               |           |             |
 *       |   5    |  (FREE)       |           |             |
 *       |________|_______________|___________|_____________|
 */
enum pipe_type {
	/* free pipe - free pipe is an uninitialized pipe without a stream
	* associated with it. It is a free DCN pipe resource. It can be
	* acquired as any type of pipe.
	*/
	FREE_PIPE,

	/* OTG master pipe - the master pipe of its OPP head pipes with a
	* functional OTG. It merges all its OPP head pipes pixel data in ODM
	* block and output to backend DIG. OTG master pipe is responsible for
	* generating entire crtc timing to backend DIG. An OTG master pipe may
	* or may not have a plane. If it has a plane it blends it as the left
	* most MPC slice of the top most layer. If it doesn't have a plane it
	* can output pixel data from its OPP head pipes' test pattern
	* generators (DPG) such as solid black pixel data to blank the screen.
	*/
	OTG_MASTER,

	/* OPP head pipe - the head pipe of an MPC blending tree with a
	* functional OPP outputting to an OTG. OPP head pipe is responsible for
	* processing output pixels in its own ODM slice. It may or may not have
	* a plane. If it has a plane it blends it as the top most layer within
	* its own ODM slice. If it doesn't have a plane it can output pixel
	* data from its DPG such as solid black pixel data to blank the pixel
	* data in its own ODM slice. OTG master pipe is also an OPP head pipe
	* but with more responsibility.
	*/
	OPP_HEAD,

	/* DPP pipe - the pipe with a functional DPP outputting to an OPP head
	* pipe's MPC. DPP pipe is responsible for processing pixel data from
	* its own MPC slice of a plane. It must be connected to an OPP head
	* pipe and it must have a plane associated with it.
	*/
	DPP_PIPE,
};

/*
 * Determine if the input pipe ctx is of a pipe type.
 * return - true if pipe ctx is of the input type.
 */
bool resource_is_pipe_type(const struct pipe_ctx *pipe_ctx, enum pipe_type type);

struct pipe_ctx *find_idle_secondary_pipe(
		struct resource_context *res_ctx,
		const struct resource_pool *pool,
		const struct pipe_ctx *primary_pipe);

bool resource_validate_attach_surfaces(
		const struct dc_validation_set set[],
		int set_count,
		const struct dc_state *old_context,
		struct dc_state *context,
		const struct resource_pool *pool);

enum dc_status resource_map_clock_resources(
		const struct dc *dc,
		struct dc_state *context,
		struct dc_stream_state *stream);

enum dc_status resource_map_phy_clock_resources(
		const struct dc *dc,
		struct dc_state *context,
		struct dc_stream_state *stream);

bool pipe_need_reprogram(
		struct pipe_ctx *pipe_ctx_old,
		struct pipe_ctx *pipe_ctx);

void resource_build_bit_depth_reduction_params(struct dc_stream_state *stream,
		struct bit_depth_reduction_params *fmt_bit_depth);

void update_audio_usage(
		struct resource_context *res_ctx,
		const struct resource_pool *pool,
		struct audio *audio,
		bool acquired);

unsigned int resource_pixel_format_to_bpp(enum surface_pixel_format format);

void get_audio_check(struct audio_info *aud_modes,
	struct audio_check *aud_chk);

int get_num_mpc_splits(struct pipe_ctx *pipe);

int get_num_odm_splits(struct pipe_ctx *pipe);

bool get_temp_dp_link_res(struct dc_link *link,
		struct link_resource *link_res,
		struct dc_link_settings *link_settings);

#if defined(CONFIG_DRM_AMD_DC_FP)
struct hpo_dp_link_encoder *resource_get_hpo_dp_link_enc_for_det_lt(
		const struct resource_context *res_ctx,
		const struct resource_pool *pool,
		const struct dc_link *link);
#endif

void reset_syncd_pipes_from_disabled_pipes(struct dc *dc,
	struct dc_state *context);

void check_syncd_pipes_for_disabled_master_pipe(struct dc *dc,
	struct dc_state *context,
	uint8_t disabled_master_pipe_idx);

void reset_sync_context_for_pipe(const struct dc *dc,
	struct dc_state *context,
	uint8_t pipe_idx);

uint8_t resource_transmitter_to_phy_idx(const struct dc *dc, enum transmitter transmitter);

const struct link_hwss *get_link_hwss(const struct dc_link *link,
		const struct link_resource *link_res);

bool is_h_timing_divisible_by_2(struct dc_stream_state *stream);

bool dc_resource_acquire_secondary_pipe_for_mpc_odm(
		const struct dc *dc,
		struct dc_state *state,
		struct pipe_ctx *pri_pipe,
		struct pipe_ctx *sec_pipe,
		bool odm);

/* A test harness interface that modifies dp encoder resources in the given dc
 * state and bypasses the need to revalidate. The interface assumes that the
 * test harness interface is called with pre-validated link config stored in the
 * pipe_ctx and updates dp encoder resources according to the link config.
 */
enum dc_status update_dp_encoder_resources_for_test_harness(const struct dc *dc,
		struct dc_state *context,
		struct pipe_ctx *pipe_ctx);
#endif /* DRIVERS_GPU_DRM_AMD_DC_DEV_DC_INC_RESOURCE_H_ */
