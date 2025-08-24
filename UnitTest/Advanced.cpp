#include <gtest/gtest.h>

#include <expected>
#include <filesystem>
#include <fstream>
#include <iterator>     // ← 使用 istreambuf_iterator 需要此標頭
#include <string>
#include <string_view>
#include <variant>

// 1. 定義 錯誤類型 & 錯誤回傳內容

struct FileNotFoundError   { std::string path; };
struct PermissionError     { std::string path; };
struct IOError             { std::string path; std::string op; };
struct BadFormatError      { std::string reason; int line; };
struct MemoryError         { std::string reason; };
struct TooManyOpenFiles    { int limit; };


// 2. 串接 錯誤類型
using Error = std::variant<
  FileNotFoundError, PermissionError, IOError,
  BadFormatError, MemoryError, TooManyOpenFiles
>;

// 建立 Visitor helper
template<class... Ts>
struct Overloaded : Ts... { using Ts::operator()...; };
template<class... Ts>
Overloaded(Ts...) -> Overloaded<Ts...>;


// 3. 建立“被測”對象
namespace demo
{
    // 縮寫
    namespace fs = std::filesystem;

    // 功能: 讀取所有檔案
    // 若檔名含 "PERM_DENIED" 則 PermissionError；
    // 若檔案內容含 "TRIGGER_IO_ERROR" → IOError（模擬）
    [[nodiscard]] std::expected<std::string, Error> ReadAll(const fs::path& p)
    {
        const auto fname = p.filename().string();

        // 檔案不存在
        if (!fs::exists(p))
            return std::unexpected(FileNotFoundError{p.string()});

        // 檔名帶有 PERM_DENIED → 模擬權限被拒
        if (fname.find("PERM_DENIED") != std::string::npos)
            return std::unexpected(PermissionError{p.string()});

        // 嘗試開檔
        std::ifstream fin(p, std::ios::binary);
        if (!fin.is_open())
            return std::unexpected(IOError{p.string(), "open"});

        // 讀取內容
        std::string content{std::istreambuf_iterator<char>(fin), {}};

        // 檢查讀取狀態：若既非 good 亦非 EOF，視為 I/O 錯誤
        if (!fin.good() && !fin.eof())
            return std::unexpected(IOError{p.string(), "read"});

        // 內容含 TRIGGER_IO_ERROR → 模擬讀取錯誤
        if (content.find("TRIGGER_IO_ERROR") != std::string::npos)
            return std::unexpected(IOError{p.string(), "read (simulated)"});

        return content;
    }

    // 功能: 解析檔案
    // 內容含 "MALFORMED" → BadFormatError
    // 若字數超過 1024 → MemoryError（模擬 out-of-memory）
    [[nodiscard]] std::expected<std::string, Error> ParseConfig(std::string content)
    {
        if (content.find("MALFORMED") != std::string::npos)
            return std::unexpected(BadFormatError{"MALFORMED token", 1});

        if (content.size() > 1024)
            return std::unexpected(MemoryError{"simulated out-of-memory"});

        // 模擬解析（做一點點轉換，實務上可忽略）
        for (char &c : content)
            c = (c == 0) ? c : static_cast<char>(c - 1);

        return content;
    }

    // 模擬「同時開太多檔案」
    [[nodiscard]] std::expected<void, Error> SimulateOpenMany(int count, int limit = 1024)
    {
        if (count >= limit)
            return std::unexpected(TooManyOpenFiles{limit});
        return {};
    }

    // 建立 Pipeline：Read → Parse
    [[nodiscard]] std::expected<std::string, Error> LoadAndParse(const fs::path& p)
    {
        return ReadAll(p).and_then(ParseConfig);
    }
}

// ---------------------------
// 4. 建立 Google 測試 Fixture
// ---------------------------
class ErrorCasesTest : public ::testing::Test
{
protected:
    std::filesystem::path dir;

    // 建立檔案並寫入內容
    std::filesystem::path make_file(const std::string& name, std::string_view content)
    {
        auto p = dir / name;
        std::ofstream ofs(p, std::ios::binary);
        ofs.write(content.data(), static_cast<std::streamsize>(content.size()));
        ofs.close();
        return p;
    }

    // Google Test 建構子
    void SetUp() override
    {
        // 建立臨時資料夾
        dir = std::filesystem::temp_directory_path() / "err_cases_demo";
        std::error_code ec;
        std::filesystem::create_directories(dir, ec);
        for (auto& e : std::filesystem::directory_iterator(dir, ec))
            std::filesystem::remove_all(e.path(), ec);
    }

    // Google Test 解構子
    void TearDown() override
    {
        std::error_code ec;
        std::filesystem::remove_all(dir, ec);
    }
};


// 5. 建立測試場景


// A. 觸發 FileNotFoundError
TEST_F(ErrorCasesTest, FileNotFound)
{
    auto path = dir / "not_exists.json"; // 保證不存在
    auto r = demo::LoadAndParse(path);

    ASSERT_FALSE(r.has_value()) << "此案例應為讀檔失敗(FileNotFound)。";
    std::visit(Overloaded{
        [&](const FileNotFoundError& e) { EXPECT_EQ(e.path, path.string()); },
        [&](const auto&) { ADD_FAILURE() << "預期 FileNotFoundError。"; }
    }, r.error());
}

// B. 觸發 PermissionError（檔名含 PERM_DENIED）
TEST_F(ErrorCasesTest, PermissionError_Read)
{
    auto path = make_file("PERM_DENIED.json", "whatever");
    auto r = demo::LoadAndParse(path);

    ASSERT_FALSE(r.has_value()) << "此案例應為權限錯誤(PERM_DENIED 模擬)。";
    std::visit(Overloaded{
        [&](const PermissionError& e) { EXPECT_EQ(e.path, path.string()); },
        [&](const auto&) { ADD_FAILURE() << "預期 PermissionError。"; }
    }, r.error());
}

// C. 觸發 I/O 讀取錯（內容含 TRIGGER_IO_ERROR）
TEST_F(ErrorCasesTest, IOError_Read)
{
    auto path = make_file("io.json", "TRIGGER_IO_ERROR");
    auto r = demo::LoadAndParse(path);

    ASSERT_FALSE(r.has_value()) << "此案例應為 I/O 錯誤（模擬 read 失敗）。";
    std::visit(Overloaded{
        [&](const IOError& e) {
            EXPECT_EQ(e.path, path.string());
            EXPECT_NE(e.op.find("read"), std::string::npos);
        },
        [&](const auto&) { ADD_FAILURE() << "預期 IOError。"; }
    }, r.error());
}

// D. 觸發 BadFormat（內容含 MALFORMED）
TEST_F(ErrorCasesTest, BadFormat)
{
    auto path = make_file("bad.json", "MALFORMED: token here");
    auto r = demo::LoadAndParse(path);

    ASSERT_FALSE(r.has_value()) << "此案例應為解析錯誤(BadFormat)。";
    std::visit(Overloaded{
        [&](const BadFormatError& e) {
            EXPECT_EQ(e.reason, "MALFORMED token");
            EXPECT_EQ(e.line, 1);
        },
        [&](const auto&) { ADD_FAILURE() << "預期 BadFormatError。"; }
    }, r.error());
}

// E. 觸發 TooManyOpenFiles（直接呼叫模擬函式）
TEST_F(ErrorCasesTest, TooManyOpenFiles)
{
    auto r = demo::SimulateOpenMany(/*count*/4096, /*limit*/1024);

    ASSERT_FALSE(r.has_value()) << "此案例應為 TooManyOpenFiles。";
    std::visit(Overloaded{
        [&](const TooManyOpenFiles& e) { EXPECT_EQ(e.limit, 1024); },
        [&](const auto&) { ADD_FAILURE() << "預期 TooManyOpenFiles。"; }
    }, r.error());
}

// F. 正向路徑（可選）：讀取 + 解析成功
TEST_F(ErrorCasesTest, HappyPath)
{
    auto path = make_file("ok.json", "HELLO"); // 不含 MALFORMED / TRIGGER_IO_ERROR
    auto r = demo::LoadAndParse(path);

    ASSERT_TRUE(r.has_value()) << "此案例應為成功路徑。";
    // 解析流程會對字元 -1，"HELLO" → "GDKKN"
    EXPECT_EQ(*r, "GDKKN");
}

// 編譯指令: g++ -std=gnu++23 Advance.cpp -lgtest_main -lgtest -pthread -o advance_tests
// 執行指令: ./advance_tests
// 結果
/*
Running main() from ./googletest/src/gtest_main.cc
[==========] Running 6 tests from 1 test suite.
[----------] Global test environment set-up.
[----------] 6 tests from ErrorCasesTest
[ RUN      ] ErrorCasesTest.FileNotFound
[       OK ] ErrorCasesTest.FileNotFound (0 ms)
[ RUN      ] ErrorCasesTest.PermissionError_Read
[       OK ] ErrorCasesTest.PermissionError_Read (0 ms)
[ RUN      ] ErrorCasesTest.IOError_Read
[       OK ] ErrorCasesTest.IOError_Read (0 ms)
[ RUN      ] ErrorCasesTest.BadFormat
[       OK ] ErrorCasesTest.BadFormat (0 ms)
[ RUN      ] ErrorCasesTest.TooManyOpenFiles
[       OK ] ErrorCasesTest.TooManyOpenFiles (0 ms)
[ RUN      ] ErrorCasesTest.HappyPath
[       OK ] ErrorCasesTest.HappyPath (0 ms)
[----------] 6 tests from ErrorCasesTest (2 ms total)

[----------] Global test environment tear-down
[==========] 6 tests from 1 test suite ran. (2 ms total)
[  PASSED  ] 6 tests.
*/

