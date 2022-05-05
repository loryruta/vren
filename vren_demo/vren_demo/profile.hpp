#pragma once


namespace vren_demo
{
	struct profile_info
	{
		uint32_t m_frame_idx;
		uint32_t m_frame_in_flight_count;

		bool m_frame_profiled;
		uint64_t m_frame_start_t;
		uint64_t m_frame_end_t;

		bool m_main_pass_profiled;
		uint64_t m_main_pass_start_t;
		uint64_t m_main_pass_end_t;

		bool m_ui_pass_profiled;
		uint64_t m_ui_pass_start_t;
		uint64_t m_ui_pass_end_t;
	};
}

