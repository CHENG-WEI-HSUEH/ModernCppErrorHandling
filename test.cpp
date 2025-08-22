#include <gtest/gtest.h>

#include <expected>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <variant>

// ---------------------------
// 1) Error types & alias
// ---------------------------
struct FileNotFoundError   { std::string path; };
struct PermissionError     { std::string path; };
struct IOError             { std::string path; std::string op; };
struct BadFormatError      { std::string reason; int line; };
struct WriteProtectError   { std::string path; };
struct MemoryError         { std::string reason; };
struct TooManyOpenFiles    { int limit; };

using Error = std::variant<
  FileNotFoundError, PermissionError, IOError,
  BadFormatError, WriteProtectError, MemoryError, TooManyOpenFiles
>;

// visitor helper
template<class... Ts>
struct Overloaded : Ts... { using Ts::operator()...; };
template<class... Ts>
Overloaded(Ts...) -> Overloaded<Ts...>;

// ---------------------------
// 2) “被測”的小型 API
// ---------------------------
namespace demo {
  namespace fs = std::filesystem;

  // Read the whole file (模擬: 檔名含 "PERM_DENIED" → PermissionError；
  // 內容含 "TRIGGER_IO_ERROR" → IOError)
  [[nodiscard]] std::expected<std::string, Error>
  ReadAll(const fs::path& p) {
    const auto fname = p.filename().string();

    if (!fs::exists(p)) {
      return std::unexpected(FileNotFoundError{p.string()});
    }
    if (fname.find("PERM_DENIED") != std::string::npos) {
      return std::unexpected(PermissionError{p.string()}); // 模擬
    }

    std::ifstream fin(p, std::ios::binary);
    if (!fin.is_open()) {
      // 無法區分細項，就以一般 I/O 失敗處理
      return std::unexpected(IOError{p.string(), "open"});
    }

    std::string content{std::istreambuf_iterator<char>(fin), {}};
    if (!fin.good() && !fin.eof()) {
      return std::unexpected(IOError{p.string(), "read"});
    }
    if (content.find("TRIGGER_IO_ERROR") != std::string::npos) {
      return std::unexpected(IOError{p.string(), "read (simulated)"});
    }
    return content;
  }

  // Parse a tiny “config”: 內容含 "MALFORMED" → BadFormat，
  // 含 "OOM" → MemoryError（模擬 out-of-memory）
  [[nodiscard]] std::expected<int, Error>
  ParseConfig(std::string_view content) {
    if (content.find("MALFORMED") != std::string_view::npos) {
      return std::unexpected(BadFormatError{"MALFORMED token", 1});
    }
    if (content.find("OOM") != std::string_view::npos) {
      return std::unexpected(MemoryError{"simulated out-of-memory"});
    }
    // 假設 config 是單純數字；不影響本例
    return static_cast<int>(content.size());
  }

  // Write file（模擬：檔名含 "WRITE_PROTECT" → WriteProtectError）
  [[nodiscard]] std::expected<void, Error>
  WriteAll(const fs::path& p, std::string_view data) {
    const auto fname = p.filename().string();
    if (fname.find("WRITE_PROTECT") != std::string::npos) {
      return std::unexpected(WriteProtectError{p.string()}); // 模擬
    }
    std::ofstream fout(p, std::ios::binary | std::ios::trunc);
    if (!fout.is_open()) {
      return std::unexpected(PermissionError{p.string()}); // 一般權限/路徑問題
    }
    fout.write(data.data(), static_cast<std::streamsize>(data.size()));
    if (!fout.good()) {
      return std::unexpected(IOError{p.string(), "write"});
    }
    return {};
  }

  // 模擬「同時開太多檔案」
  [[nodiscard]] std::expected<void, Error>
  SimulateOpenMany(int count, int limit = 1024) {
    if (count >= limit) {
      return std::unexpected(TooManyOpenFiles{limit});
    }
    return {};
  }

  // 一個小管線：Read → Parse
  [[nodiscard]] std::expected<int, Error>
  LoadAndParse(const fs::path& p) {
    return ReadAll(p).and_then(ParseConfig);
  }
}

// ---------------------------
// 3) Test Fixture
// ---------------------------
class ErrorCasesTest : public ::testing::Test {
protected:
  std::filesystem::path dir;
  std::filesystem::path make_file(const std::string& name,
                                  std::string_view content) {
    auto p = dir / name;
    std::ofstream(p) << content;
    return p;
  }

  void SetUp() override {
    dir = std::filesystem::temp_directory_path() / "err_cases_demo";
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    // 清理舊檔
    for (auto& e : std::filesystem::directory_iterator(dir, ec)) {
      std::filesystem::remove_all(e.path(), ec);
    }
  }

  void TearDown() override {
    std::error_code ec;
    std::filesystem::remove_all(dir, ec);
  }
};

// ---------------------------
// 4) Tests
// ---------------------------

TEST_F(ErrorCasesTest, FileNotFound) {
  auto path = dir / "not_exists.json"; // 保證不存在
  auto r = demo::LoadAndParse(path);
  ASSERT_FALSE(r.has_value());

  // 使用 variant + visit 檢查具體錯誤型別
  std::visit(Overloaded{
    [&](const FileNotFoundError& e) {
      EXPECT_EQ(e.path, path.string());
    },
    [&](const auto&) {
      ADD_FAILURE() << "Expected FileNotFoundError";
    }
  }, r.error());
}

TEST_F(ErrorCasesTest, PermissionError_Read) {
  // 檔名帶 PERM_DENIED → 在 ReadAll() 模擬成 PermissionError
  auto path = make_file("PERM_DENIED.json", "whatever");
  auto r = demo::LoadAndParse(path);
  ASSERT_FALSE(r.has_value());

  std::visit(Overloaded{
    [&](const PermissionError& e) {
      EXPECT_EQ(e.path, path.string());
    },
    [&](const auto&) {
      ADD_FAILURE() << "Expected PermissionError";
    }
  }, r.error());
}

TEST_F(ErrorCasesTest, IOError_Read) {
  auto path = make_file("io.json", "TRIGGER_IO_ERROR"); // 內容觸發 I/O 讀取錯
  auto r = demo::LoadAndParse(path);
  ASSERT_FALSE(r.has_value());

  std::visit(Overloaded{
    [&](const IOError& e) {
      EXPECT_EQ(e.path, path.string());
      EXPECT_NE(e.op.find("read"), std::string::npos);
    },
    [&](const auto&) {
      ADD_FAILURE() << "Expected IOError";
    }
  }, r.error());
}

TEST_F(ErrorCasesTest, BadFormat) {
  auto path = make_file("bad.json", "MALFORMED: token here");
  auto r = demo::LoadAndParse(path);
  ASSERT_FALSE(r.has_value());

  std::visit(Overloaded{
    [&](const BadFormatError& e) {
      EXPECT_EQ(e.reason, "MALFORMED token");
      EXPECT_EQ(e.line, 1);
    },
    [&](const auto&) {
      ADD_FAILURE() << "Expected BadFormatError";
    }
  }, r.error());
}

TEST_F(ErrorCasesTest, WriteProtectError) {
  // WriteAll() 看到檔名含 WRITE_PROTECT → 模擬 WriteProtectError
  auto path = dir / "WRITE_PROTECT.txt";
  auto r = demo::WriteAll(path, "data");
  ASSERT_FALSE(r.has_value());

  std::visit(Overloaded{
    [&](const WriteProtectError& e) {
      EXPECT_EQ(e.path, path.string());
    },
    [&](const auto&) {
      ADD_FAILURE() << "Expected WriteProtectError";
    }
  }, r.error());
}

TEST_F(ErrorCasesTest, MemoryError_Parse) {
  auto path = make_file("cfg.json", "OOM please");
  auto r = demo::LoadAndParse(path);
  ASSERT_FALSE(r.has_value());

  std::visit(Overloaded{
    [&](const MemoryError& e) {
      EXPECT_NE(e.reason.find("out-of-memory"), std::string::npos);
    },
    [&](const auto&) {
      ADD_FAILURE() << "Expected MemoryError";
    }
  }, r.error());
}

TEST_F(ErrorCasesTest, TooManyOpenFiles) {
  // 直接呼叫模擬函式
  auto r = demo::SimulateOpenMany(/*count*/4096, /*limit*/1024);
  ASSERT_FALSE(r.has_value());

  std::visit(Overloaded{
    [&](const TooManyOpenFiles& e) {
      EXPECT_EQ(e.limit, 1024);
    },
    [&](const auto&) {
      ADD_FAILURE() << "Expected TooManyOpenFiles";
    }
  }, r.error());
}

// 如果你連結 gtest_main，就不需要自訂 main()
// 這裡保留 main()，方便只連 -lgtest 時也能執行。
int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
