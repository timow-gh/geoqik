// Exercises the log and replay API via the IPC client (plan 024).
// Requires GEOQIK_EXE_PATH set to geoqik_serverd.exe.
//
// Flow:
//   1. Adds ten points and saves the log to a temp file.
//   2. Loads the log back (replaces the in-memory log).
//   3. Replays the current log in a paused state.
//   4. Steps forward, checks progress, steps backward, resumes, pauses, cancels.
//   5. Replays the saved file with default options, then exits.

#include <GeoQikClient/GeoQikClient.hpp>
#include <cassert>
#include <cstdio>
#include <filesystem>
#include <iostream>
#include <string>

int main() {
    [[maybe_unused]] auto err = geoqik_init();
    assert(err == GEOQIK_SUCCESS && "geoqik_init failed — is GEOQIK_EXE_PATH set?");

    err = geoqik_draw();
    assert(err == GEOQIK_SUCCESS);

    for (int i = 0; i < 10; ++i) {
        [[maybe_unused]] const auto pr = geoqik_add_point(static_cast<double>(i) * 0.1, 0.0, 0.0);
        assert(pr.err == GEOQIK_SUCCESS);
    }

    const std::string logPath =
        (std::filesystem::temp_directory_path() / "geoqik_plan024_test.gql").string();

    err = geoqik_save_log(logPath.c_str(), GEOQIK_LOG_FORMAT_BINARY);
    assert(err == GEOQIK_SUCCESS && "geoqik_save_log failed");

    err = geoqik_load_log(logPath.c_str(), GEOQIK_LOG_FORMAT_BINARY);
    assert(err == GEOQIK_SUCCESS && "geoqik_load_log failed");

    geoqik_replay_options_t opts{};
    opts.startPaused = 1;
    err = geoqik_replay_current_log(&opts);
    assert(err == GEOQIK_SUCCESS && "geoqik_replay_current_log failed");

    geoqik_replay_state_t state = GEOQIK_REPLAY_INACTIVE;
    err = geoqik_get_replay_state(&state);
    assert(err == GEOQIK_SUCCESS);
    assert(state == GEOQIK_REPLAY_PAUSED);

    err = geoqik_step_replay_n(3);
    assert(err == GEOQIK_SUCCESS);

    std::size_t current = 0, total = 0;
    err = geoqik_get_replay_progress(&current, &total);
    assert(err == GEOQIK_SUCCESS);
    assert(current > 0 && total > 0);
    std::cout << "Replay progress: " << current << " / " << total << '\n';

    err = geoqik_step_replay_backward();
    assert(err == GEOQIK_SUCCESS);

    err = geoqik_resume_replay();
    assert(err == GEOQIK_SUCCESS);
    err = geoqik_get_replay_state(&state);
    assert(err == GEOQIK_SUCCESS);
    assert(state == GEOQIK_REPLAY_PLAYING);

    err = geoqik_pause_replay();
    assert(err == GEOQIK_SUCCESS);

    err = geoqik_cancel_replay();
    assert(err == GEOQIK_SUCCESS);
    err = geoqik_get_replay_state(&state);
    assert(err == GEOQIK_SUCCESS);
    assert(state == GEOQIK_REPLAY_INACTIVE);

    // Replay directly from the saved file with all-default options.
    err = geoqik_replay_log(logPath.c_str(), GEOQIK_LOG_FORMAT_BINARY, nullptr);
    assert(err == GEOQIK_SUCCESS && "geoqik_replay_log failed");

    std::remove(logPath.c_str());

    std::cout << "Plan 024 log/replay test complete — close the window to exit.\n";

    err = geoqik_wait_for_exit_and_cleanup();
    assert(err == GEOQIK_SUCCESS);
    return 0;
}
