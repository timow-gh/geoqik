#include <GeoQikClient/GeoQikClient.hpp>
#include <gtest/gtest.h>
#include <cstdlib>
#include <string>

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

// --- Error-path tests -------------------------------------------------------

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
