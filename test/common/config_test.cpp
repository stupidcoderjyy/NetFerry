#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>

#include <gtest/gtest.h>

#include "core/common/config.h"

namespace {

class ConfigTest : public ::testing::Test {
protected:
    void SetUp() override {
        tmp_dir_ = std::filesystem::temp_directory_path() / "net_ferry_test_config";
        std::filesystem::create_directories(tmp_dir_);
    }

    void TearDown() override {
        std::filesystem::remove_all(tmp_dir_);
    }

    void WriteConfigFile(const std::string& name, const std::string& content) {
        auto path = tmp_dir_ / name;
        std::ofstream ofs(path);
        ofs << content;
        ofs.close();
    }

    std::filesystem::path tmp_dir_;
};

TEST_F(ConfigTest, LoadValidInsideConfig) {
    WriteConfigFile("inside.json", R"({
        "role": "inside",
        "listen_port": 8080,
        "inbox_dir": "/data/inbox",
        "outbox_dir": "/data/outbox",
        "file_ext_req": ".req",
        "file_ext_resp": ".resp"
    })");

    auto config = net_ferry::common::Config::LoadFromFile(
        (tmp_dir_ / "inside.json").string());

    EXPECT_EQ(config.role(), "inside");
    EXPECT_EQ(config.listen_port(), 8080);
    EXPECT_EQ(config.inbox_dir(), "/data/inbox");
    EXPECT_EQ(config.outbox_dir(), "/data/outbox");
    EXPECT_EQ(config.file_ext_req(), ".req");
    EXPECT_EQ(config.file_ext_resp(), ".resp");
}

TEST_F(ConfigTest, LoadValidOutsideConfig) {
    WriteConfigFile("outside.json", R"({
        "role": "outside",
        "inbox_dir": "/data/inbox",
        "outbox_dir": "/data/outbox",
        "target_server": "http://real-backend:9090",
        "file_ext_req": ".req",
        "file_ext_resp": ".resp"
    })");

    auto config = net_ferry::common::Config::LoadFromFile(
        (tmp_dir_ / "outside.json").string());

    EXPECT_EQ(config.role(), "outside");
    EXPECT_EQ(config.target_server(), "http://real-backend:9090");
}

TEST_F(ConfigTest, DefaultExtensions) {
    WriteConfigFile("defaults.json", R"({
        "role": "inside",
        "inbox_dir": "/tmp/in",
        "outbox_dir": "/tmp/out"
    })");

    auto config = net_ferry::common::Config::LoadFromFile(
        (tmp_dir_ / "defaults.json").string());

    EXPECT_EQ(config.file_ext_req(), ".req");
    EXPECT_EQ(config.file_ext_resp(), ".resp");
    EXPECT_EQ(config.listen_port(), 0);
    EXPECT_EQ(config.target_server(), "");
}

TEST_F(ConfigTest, MissingRoleThrows) {
    WriteConfigFile("no_role.json", R"({
        "inbox_dir": "/tmp/in",
        "outbox_dir": "/tmp/out"
    })");

    EXPECT_THROW(
        net_ferry::common::Config::LoadFromFile(
            (tmp_dir_ / "no_role.json").string()),
        std::runtime_error);
}

TEST_F(ConfigTest, MissingInboxThrows) {
    WriteConfigFile("no_inbox.json", R"({
        "role": "inside",
        "outbox_dir": "/tmp/out"
    })");

    EXPECT_THROW(
        net_ferry::common::Config::LoadFromFile(
            (tmp_dir_ / "no_inbox.json").string()),
        std::runtime_error);
}

TEST_F(ConfigTest, MissingOutboxThrows) {
    WriteConfigFile("no_outbox.json", R"({
        "role": "inside",
        "inbox_dir": "/tmp/in"
    })");

    EXPECT_THROW(
        net_ferry::common::Config::LoadFromFile(
            (tmp_dir_ / "no_outbox.json").string()),
        std::runtime_error);
}

TEST_F(ConfigTest, FileNotFoundThrows) {
    EXPECT_THROW(
        net_ferry::common::Config::LoadFromFile("/nonexistent/path.json"),
        std::runtime_error);
}

}  // namespace
