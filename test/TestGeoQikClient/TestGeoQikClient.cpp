#include <GeoQikClient/GeoQikClient.hpp>
#include <gtest/gtest.h>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>
#include <thread>

// GEOQIK_SERVER_EXE_PATH is injected at compile time by CMakeLists.txt.
// It is the absolute path to the geoqik_server executable in the build tree.
#ifndef GEOQIK_SERVER_EXE_PATH
#  error "GEOQIK_SERVER_EXE_PATH must be defined by the build system"
#endif

namespace {

void set_server_path() {
#ifdef _WIN32
    ::SetEnvironmentVariableA("GEOQIK_EXE_PATH", GEOQIK_SERVER_EXE_PATH);
#else
    ::setenv("GEOQIK_EXE_PATH", GEOQIK_SERVER_EXE_PATH, /*overwrite=*/1);
#endif
}

::testing::AssertionResult wait_for_replay_state(
    const geoqik_replay_state_t expected,
    const std::chrono::milliseconds timeout = std::chrono::seconds(1)) {
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    geoqik_replay_state_t state = GEOQIK_REPLAY_INACTIVE;
    geoqik_error_code_t err = GEOQIK_SUCCESS;

    do {
        err = geoqik_get_replay_state(&state);
        if (err == GEOQIK_SUCCESS && state == expected) {
            return ::testing::AssertionSuccess();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    } while (std::chrono::steady_clock::now() < deadline);

    return ::testing::AssertionFailure()
        << "Expected replay state " << static_cast<int>(expected)
        << ", last state was " << static_cast<int>(state)
        << ", last error was " << static_cast<int>(err);
}

} // namespace

class GeoQikClientTest : public ::testing::Test {
protected:
    void SetUp() override {
        set_server_path();
    }
    void TearDown() override {
        // Best-effort cleanup — the server may already have exited.
        [[maybe_unused]] auto res = geoqik_cleanup();
        geoqik_client_clear_last_error();
    }
};

// --- Happy-path tests -------------------------------------------------------

TEST_F(GeoQikClientTest, InitAndCleanup) {
    ASSERT_EQ(GEOQIK_SUCCESS, geoqik_init());
    EXPECT_EQ(GEOQIK_SUCCESS, geoqik_cleanup());
}

TEST_F(GeoQikClientTest, InitWithSettingsReturnsSuccess) {
    geoqik_settings_t settings{};
    geoqik_create_default_settings(&settings);
    geoqik_window_settings_t winSettings{};
    geoqik_init_default_window_settings(&winSettings);

    EXPECT_EQ(GEOQIK_SUCCESS, geoqik_init_with_settings(&settings, &winSettings));
}

TEST_F(GeoQikClientTest, IsApiInitializedReturnsTrue) {
    ASSERT_EQ(GEOQIK_SUCCESS, geoqik_init());
    bool initialized = false;
    EXPECT_EQ(GEOQIK_SUCCESS, geoqik_is_api_initialized(&initialized));
    EXPECT_TRUE(initialized);
}

TEST_F(GeoQikClientTest, AddPointReturnsSuccessAndUuid) {
    ASSERT_EQ(GEOQIK_SUCCESS, geoqik_init());

    const auto result = geoqik_add_point(1.0, 2.0, 3.0);
    EXPECT_EQ(GEOQIK_SUCCESS, result.err);

    // UUID must be non-zero.
    bool allZero = true;
    for (const auto byte : result.geometryId.value) {
        if (byte != 0) { allZero = false; break; }
    }
    EXPECT_FALSE(allZero) << "Expected a non-zero UUID from geoqik_add_point";
}

TEST_F(GeoQikClientTest, AddLineReturnsSuccess) {
    ASSERT_EQ(GEOQIK_SUCCESS, geoqik_init());
    EXPECT_EQ(GEOQIK_SUCCESS,
        geoqik_add_line(0.0, 0.0, 0.0, 1.0, 1.0, 1.0));
}

TEST_F(GeoQikClientTest, DrawReturnsSuccess) {
    ASSERT_EQ(GEOQIK_SUCCESS, geoqik_init());
    EXPECT_EQ(GEOQIK_SUCCESS, geoqik_draw());
}

// DISABLED: geoqik_wait_for_exit_and_cleanup() blocks until the user closes the
// server's viewer window, so this test hangs in an unattended/CI run. Re-enable
// once the server supports a headless/auto-close mode.
TEST_F(GeoQikClientTest, DISABLED_WaitForExitAndCleanupExitsServer) {
    ASSERT_EQ(GEOQIK_SUCCESS, geoqik_init());
    EXPECT_EQ(GEOQIK_SUCCESS, geoqik_wait_for_exit_and_cleanup());
    // TearDown calls geoqik_cleanup() — should not crash even though server has exited.
}

TEST_F(GeoQikClientTest, DoubleInitIsIdempotent) {
    ASSERT_EQ(GEOQIK_SUCCESS, geoqik_init());
    // Second call must not start a new server or corrupt state.
    EXPECT_EQ(GEOQIK_SUCCESS, geoqik_init());
    // Operations after double-init must still work.
    EXPECT_EQ(GEOQIK_SUCCESS, geoqik_add_point(0.0, 0.0, 0.0).err);
}

TEST_F(GeoQikClientTest, SaveLogCreatesFile) {
    const std::filesystem::path path =
        std::filesystem::temp_directory_path() / "geoqik_client_test_save.gqklog";
    std::filesystem::remove(path);

    ASSERT_EQ(GEOQIK_SUCCESS, geoqik_init());
    ASSERT_EQ(GEOQIK_SUCCESS, geoqik_add_point(1.0, 2.0, 3.0).err);

    EXPECT_EQ(GEOQIK_SUCCESS, geoqik_save_log(path.string().c_str(), GEOQIK_LOG_FORMAT_BINARY));
    EXPECT_TRUE(std::filesystem::exists(path));
    EXPECT_GT(std::filesystem::file_size(path), 0u);

    std::filesystem::remove(path);
}

TEST_F(GeoQikClientTest, LoadLogRoundTrip) {
    const std::filesystem::path path =
        std::filesystem::temp_directory_path() / "geoqik_client_test_load.gqklog";
    std::filesystem::remove(path);

    ASSERT_EQ(GEOQIK_SUCCESS, geoqik_init());
    ASSERT_EQ(GEOQIK_SUCCESS, geoqik_add_point(0.0, 0.0, 0.0).err);
    ASSERT_EQ(GEOQIK_SUCCESS, geoqik_save_log(path.string().c_str(), GEOQIK_LOG_FORMAT_BINARY));
    ASSERT_EQ(GEOQIK_SUCCESS, geoqik_remove_all_geometry());
    EXPECT_EQ(GEOQIK_SUCCESS, geoqik_load_log(path.string().c_str(), GEOQIK_LOG_FORMAT_BINARY));

    std::filesystem::remove(path);
}

TEST_F(GeoQikClientTest, GetReplayStateReturnsInactiveWhenNoReplay) {
    ASSERT_EQ(GEOQIK_SUCCESS, geoqik_init());
    geoqik_replay_state_t state = GEOQIK_REPLAY_PLAYING;
    EXPECT_EQ(GEOQIK_SUCCESS, geoqik_get_replay_state(&state));
    EXPECT_EQ(GEOQIK_REPLAY_INACTIVE, state);
}

TEST_F(GeoQikClientTest, GetReplayProgressReturnsZeroWhenNoReplay) {
    ASSERT_EQ(GEOQIK_SUCCESS, geoqik_init());
    std::size_t current = 99;
    std::size_t total = 99;
    EXPECT_EQ(GEOQIK_SUCCESS, geoqik_get_replay_progress(&current, &total));
    EXPECT_EQ(0u, current);
    EXPECT_EQ(0u, total);
}

TEST_F(GeoQikClientTest, ReplayCurrentLogAndCancelViaIpc) {
    ASSERT_EQ(GEOQIK_SUCCESS, geoqik_init());

    ASSERT_EQ(GEOQIK_SUCCESS, geoqik_add_point(0.0, 0.0, 0.0).err);
    ASSERT_EQ(GEOQIK_SUCCESS, geoqik_add_point(1.0, 0.0, 0.0).err);
    ASSERT_EQ(GEOQIK_SUCCESS, geoqik_add_point(2.0, 0.0, 0.0).err);

    geoqik_replay_options_t opts{};
    opts.startPaused = 1;
    ASSERT_EQ(GEOQIK_SUCCESS, geoqik_replay_current_log(&opts));
    EXPECT_TRUE(wait_for_replay_state(GEOQIK_REPLAY_PAUSED));

    EXPECT_EQ(GEOQIK_SUCCESS, geoqik_cancel_replay());
    EXPECT_TRUE(wait_for_replay_state(GEOQIK_REPLAY_INACTIVE));
}

TEST_F(GeoQikClientTest, ReplayLogFromFileViaIpc) {
    const std::filesystem::path path =
        std::filesystem::temp_directory_path() / "geoqik_client_test_replay.gqklog";
    std::filesystem::remove(path);

    ASSERT_EQ(GEOQIK_SUCCESS, geoqik_init());
    ASSERT_EQ(GEOQIK_SUCCESS, geoqik_add_point(0.0, 0.0, 0.0).err);
    ASSERT_EQ(GEOQIK_SUCCESS, geoqik_save_log(path.string().c_str(), GEOQIK_LOG_FORMAT_BINARY));

    geoqik_replay_options_t opts{};
    opts.startPaused = 1;
    EXPECT_EQ(GEOQIK_SUCCESS,
        geoqik_replay_log(path.string().c_str(), GEOQIK_LOG_FORMAT_BINARY, &opts));
    EXPECT_TRUE(wait_for_replay_state(GEOQIK_REPLAY_PAUSED));

    EXPECT_EQ(GEOQIK_SUCCESS, geoqik_cancel_replay());

    std::filesystem::remove(path);
}

TEST_F(GeoQikClientTest, ReplayPauseResumeStepViaIpc) {
    ASSERT_EQ(GEOQIK_SUCCESS, geoqik_init());
    ASSERT_EQ(GEOQIK_SUCCESS, geoqik_add_point(0.0, 0.0, 0.0).err);
    ASSERT_EQ(GEOQIK_SUCCESS, geoqik_add_point(1.0, 0.0, 0.0).err);

    geoqik_replay_options_t opts{};
    opts.startPaused = 1;
    ASSERT_EQ(GEOQIK_SUCCESS, geoqik_replay_current_log(&opts));
    ASSERT_TRUE(wait_for_replay_state(GEOQIK_REPLAY_PAUSED));

    EXPECT_EQ(GEOQIK_SUCCESS, geoqik_step_replay());
    EXPECT_EQ(GEOQIK_SUCCESS, geoqik_step_replay_n(1));
    EXPECT_EQ(GEOQIK_SUCCESS, geoqik_step_replay_backward());
    EXPECT_EQ(GEOQIK_SUCCESS, geoqik_step_replay_backward_n(1));

    EXPECT_EQ(GEOQIK_SUCCESS, geoqik_resume_replay());
    EXPECT_TRUE(wait_for_replay_state(GEOQIK_REPLAY_PLAYING));

    EXPECT_EQ(GEOQIK_SUCCESS, geoqik_pause_replay());
    EXPECT_TRUE(wait_for_replay_state(GEOQIK_REPLAY_PAUSED));

    EXPECT_EQ(GEOQIK_SUCCESS, geoqik_cancel_replay());
}

TEST_F(GeoQikClientTest, ReplayOptionsWithCustomKeysViaIpc) {
    ASSERT_EQ(GEOQIK_SUCCESS, geoqik_init());
    ASSERT_EQ(GEOQIK_SUCCESS, geoqik_add_point(0.0, 0.0, 0.0).err);

    geoqik_key_t stepKey = GEOQIK_KEY_J;
    geoqik_key_t backKey = GEOQIK_KEY_K;
    geoqik_key_t resumeKey = GEOQIK_KEY_L;
    geoqik_key_t pauseKey = GEOQIK_KEY_M;
    geoqik_key_t incKey = GEOQIK_KEY_N;
    geoqik_key_t decKey = GEOQIK_KEY_O;

    geoqik_replay_options_t opts{};
    opts.startPaused = 1;
    opts.entriesPerSecond = 30.0;
    opts.speedMultiplier = 2.0;
    opts.maxEntriesPerFrame = 4;
    opts.entriesPerStep = 2;
    opts.stepKeys = &stepKey;
    opts.stepKeyCount = 1;
    opts.backwardStepKeys = &backKey;
    opts.backwardStepKeyCount = 1;
    opts.resumeKeys = &resumeKey;
    opts.resumeKeyCount = 1;
    opts.pauseKeys = &pauseKey;
    opts.pauseKeyCount = 1;
    opts.increaseEntriesPerStepKeys = &incKey;
    opts.increaseEntriesPerStepKeyCount = 1;
    opts.decreaseEntriesPerStepKeys = &decKey;
    opts.decreaseEntriesPerStepKeyCount = 1;

    EXPECT_EQ(GEOQIK_SUCCESS, geoqik_replay_current_log(&opts));
    EXPECT_EQ(GEOQIK_SUCCESS, geoqik_cancel_replay());
}

// --- Error-path tests -------------------------------------------------------

TEST_F(GeoQikClientTest, SaveLogNullPathSetsClientError) {
    ASSERT_EQ(GEOQIK_SUCCESS, geoqik_init());
    EXPECT_EQ(GEOQIK_ERROR_INVALID_PARAMETER,
        geoqik_save_log(nullptr, GEOQIK_LOG_FORMAT_BINARY));
}

TEST_F(GeoQikClientTest, ServerNotFoundSetsClientError) {
#ifdef _WIN32
    ::SetEnvironmentVariableA("GEOQIK_EXE_PATH", "nonexistent_geoqik_binary.exe");
#else
    ::setenv("GEOQIK_EXE_PATH", "nonexistent_geoqik_binary", /*overwrite=*/1);
#endif

    const auto err = geoqik_init();
    EXPECT_NE(GEOQIK_SUCCESS, err);

    geoqik_client_error_info_t info{};
    info.struct_size = sizeof(info);
    ASSERT_EQ(GEOQIK_SUCCESS, geoqik_client_get_last_error_info(&info));
    EXPECT_EQ(GEOQIK_CLIENT_ERROR_SERVER_EXECUTABLE_NOT_FOUND, info.client_code);
    EXPECT_NE(nullptr, info.what);
    EXPECT_NE(nullptr, info.why);
    EXPECT_NE(nullptr, info.action);

    // Restore correct path for TearDown.
    set_server_path();
}

// DISABLED: depends on geoqik_wait_for_exit_and_cleanup(), which blocks until the
// user closes the server's viewer window, so this test hangs in an unattended/CI
// run. Re-enable once the server supports a headless/auto-close mode.
TEST_F(GeoQikClientTest, DISABLED_CallAfterServerExitReturnsIpcError) {
    ASSERT_EQ(GEOQIK_SUCCESS, geoqik_init());
    ASSERT_EQ(GEOQIK_SUCCESS, geoqik_wait_for_exit_and_cleanup());

    // After the server has exited, the next call should detect the broken
    // connection and return a client IPC error, not crash or hang.
    const auto err = geoqik_add_point(0.0, 0.0, 0.0).err;
    EXPECT_NE(GEOQIK_SUCCESS, err);

    geoqik_client_error_info_t info{};
    info.struct_size = sizeof(info);
    ASSERT_EQ(GEOQIK_SUCCESS, geoqik_client_get_last_error_info(&info));
    // Should be a connection or write error, not success.
    EXPECT_NE(GEOQIK_CLIENT_SUCCESS, info.client_code);
}
