// Basic.cpp
#include <gtest/gtest.h>
#include "Config.h"            // 與專案內部檔案一致，使用雙引號
#include <expected>
#include <filesystem>
#include <fstream>
#include <string>
#include <variant>
#include <iostream>

using namespace std;
using namespace std::string_literals;
using namespace config;        // 直接使用 config 命名空間中的 API/型別

// ---------------------------
// 測試用工具函式
// ---------------------------

// 建立檔案並寫入指定內容；回傳建立好的完整路徑
static std::filesystem::path make_file_with(
    const std::filesystem::path& dir,
    const std::string& filename,
    const std::string& content)
{
    std::filesystem::create_directories(dir);
    auto p = dir / filename;
    std::ofstream ofs(p, std::ios::binary);
    ofs << content;
    ofs.close();
    return p;
}

// ---------------------------
// 測試 Fixture
// ---------------------------
class ErrorCasesTest : public ::testing::Test {
protected:
    std::filesystem::path dir;

    // Google Test 初始化：建立臨時目錄
    void SetUp() override {
        // 使用作業系統臨時資料夾底下建立專屬子目錄
        dir = std::filesystem::temp_directory_path() / "cfg_pipeline_tests";
        std::error_code ec;
        std::filesystem::create_directories(dir, ec);
    }

    // Google Test 解構：清理臨時目錄
    void TearDown() override {
        std::error_code ec;
        std::filesystem::remove_all(dir, ec);
    }
};

// ---------------------------------------------------------
// 情境一：讀取不存在的檔案 -> 觸發 ConfigReadError
// ---------------------------------------------------------
TEST_F(ErrorCasesTest, ReadMissingFile_Triggers_ConfigReadError) {
    // 建立一個必定不存在的路徑
    auto path = dir / "does_not_exist.json";

    auto res = LoadConfig(path.string()); // 應該回傳 unexpected(ConfigReadError)  :contentReference[oaicite:2]{index=2}
    ASSERT_FALSE(res.has_value()) << "此情境應為讀檔失敗";

    // 使用 Overloaded + std::visit 檢查錯誤型別
    std::visit(Overloaded{
        [&](const ConfigReadError& e) {
            // 驗證錯誤中帶有的檔名
            EXPECT_EQ(e.filename, path.string());
        },
        [&](const auto&) {
            ADD_FAILURE() << "預期為 ConfigReadError，但收到其他錯誤型別";
        }
    }, res.error());
}

// ---------------------------------------------------------
// 情境二：讀到「malformed」內容 -> 觸發 ConfigParseError
// 註：你的 LoadConfig 會在內容為空或包含 "malformed" 時回傳解析錯誤
// ---------------------------------------------------------
TEST_F(ErrorCasesTest, MalformedContent_Triggers_ConfigParseError) {
    // 產生一個包含 "malformed" 的檔案
    auto p = make_file_with(dir, "bad.cfg", "this is malformed configuration");
    auto res = LoadConfig(p.string()); // 預期解析錯誤  :contentReference[oaicite:3]{index=3}
    ASSERT_FALSE(res.has_value()) << "此情境應為解析失敗";

    std::visit(Overloaded{
        [&](const ConfigParseError& e) {
            // 驗證錯誤內容帶有我們預期的片段與行號（目前實作使用行號 1）
            EXPECT_EQ(e.line_content, "malformed");
            EXPECT_EQ(e.line_number, 1);
        },
        [&](const auto&) {
            ADD_FAILURE() << "預期為 ConfigParseError，但收到其他錯誤型別";
        }
    }, res.error());
}

// ---------------------------------------------------------
// 情境三：內容含 invalid_field -> 驗證失敗 ValidationError
// 管線：LoadConfig -> ValidateData
// ---------------------------------------------------------
TEST_F(ErrorCasesTest, InvalidField_Triggers_ValidationError) {
    // 內容包含 invalid_field（但避免 "malformed" 關鍵字讓它停在 Parse）
    auto p = make_file_with(dir, "invalid.cfg", R"(key=ok; invalid_field=bad;)");
    auto res = LoadConfig(p.string())
        .and_then([](const Config& cfg) { return ValidateData(cfg); }); // 期望在此步驟失敗  :contentReference[oaicite:4]{index=4}

    ASSERT_FALSE(res.has_value()) << "此情境應為驗證失敗";

    std::visit(Overloaded{
        [&](const ValidationError& e) {
            EXPECT_EQ(e.field_name, "invalid_field");
            // e.invalid_value 的語義為說明字串，僅檢查非空
            EXPECT_FALSE(e.invalid_value.empty());
        },
        [&](const auto&) {
            ADD_FAILURE() << "預期為 ValidationError，但收到其他錯誤型別";
        }
    }, res.error());
}

// ---------------------------------------------------------
// 情境四：處理階段資料太短 -> 觸發 ProcessingError
//
// 說明：依目前實作，ValidateData 會回傳 "Validated: " + 原字串，
// 使處理前字串長度至少 11，正常管線下不可能 < 10，無法觸發 ProcessingError。
// 因此此測試「直接」構造極短的 ValidatedData 後呼叫 ProcessData，
// 以覆蓋該錯誤分支並驗證錯誤型別正確。                 :contentReference[oaicite:5]{index=5}
// ---------------------------------------------------------
TEST_F(ErrorCasesTest, TooShortData_Triggers_ProcessingError) {
    ValidatedData very_short{ "x" }; // 刻意過短
    auto res = ProcessData(very_short); // 預期為 ProcessingError（資料太短）  :contentReference[oaicite:6]{index=6}

    ASSERT_FALSE(res.has_value()) << "此情境應為處理失敗";

    std::visit(Overloaded{
        [&](const ProcessingError& e) {
            EXPECT_EQ(e.task_name, "Data Processing");
            EXPECT_FALSE(e.details.empty());
        },
        [&](const auto&) {
            ADD_FAILURE() << "預期為 ProcessingError，但收到其他錯誤型別";
        }
    }, res.error());
}

// 編譯: g++ -std=gnu++23 Basic.cpp Config.cpp -I. -lgtest_main -lgtest -pthread -o test_basic
// 執行: ./test_basic

// 執行結果如下
/*
Running main() from ./googletest/src/gtest_main.cc
[==========] Running 1 test from 1 test suite.
[----------] Global test environment set-up.
[----------] 1 test from ErrorCasesTest
[ RUN      ] ErrorCasesTest.ReadMissingFile_Triggers_ConfigReadError
DEBUG: LoadConfig failed to open /tmp/cfg_pipeline_tests/does_not_exist.json
[       OK ] ErrorCasesTest.ReadMissingFile_Triggers_ConfigReadError (2 ms)
[----------] 1 test from ErrorCasesTest (2 ms total)

[----------] Global test environment tear-down
[==========] 1 test from 1 test suite ran. (2 ms total)
[  PASSED  ] 1 test.
hsueh@HsuehChengWei:/mnt/c/Users/Administrator/Documents/Smartsurgery$ g++ -std=gnu++23 test.cpp Config.cpp -I. -lgtest_main -lgtest -pthread -o test_basic
hsueh@HsuehChengWei:/mnt/c/Users/Administrator/Documents/Smartsurgery$ ./test_basic
Running main() from ./googletest/src/gtest_main.cc
[==========] Running 4 tests from 1 test suite.
[----------] Global test environment set-up.
[----------] 4 tests from ErrorCasesTest
[ RUN      ] ErrorCasesTest.ReadMissingFile_Triggers_ConfigReadError
DEBUG: LoadConfig failed to open /tmp/cfg_pipeline_tests/does_not_exist.json
[       OK ] ErrorCasesTest.ReadMissingFile_Triggers_ConfigReadError (0 ms)
[ RUN      ] ErrorCasesTest.MalformedContent_Triggers_ConfigParseError
DEBUG: LoadConfig detected malformed config in /tmp/cfg_pipeline_tests/bad.cfg
[       OK ] ErrorCasesTest.MalformedContent_Triggers_ConfigParseError (0 ms)
[ RUN      ] ErrorCasesTest.InvalidField_Triggers_ValidationError
DEBUG: Config loaded successfully from /tmp/cfg_pipeline_tests/invalid.cfg
DEBUG: ValidateData detected invalid field.
[       OK ] ErrorCasesTest.InvalidField_Triggers_ValidationError (0 ms)
[ RUN      ] ErrorCasesTest.TooShortData_Triggers_ProcessingError
DEBUG: ProcessData detected data too short.
[       OK ] ErrorCasesTest.TooShortData_Triggers_ProcessingError (0 ms)
[----------] 4 tests from ErrorCasesTest (1 ms total)

[----------] Global test environment tear-down
[==========] 4 tests from 1 test suite ran. (1 ms total)
[  PASSED  ] 4 tests.

*/
