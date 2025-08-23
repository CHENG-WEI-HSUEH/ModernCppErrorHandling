#include <gtest/gtest.h>

#include <expected>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <variant>

// ---------------------------
// 1. 定義 錯類類型&錯誤回傳內容
// ---------------------------
struct FileNotFoundError   { std::string path; };
struct PermissionError     { std::string path; };
struct IOError             { std::string path; std::string op; };
struct BadFormatError      { std::string reason; int line; };
struct MemoryError         { std::string reason; };
struct TooManyOpenFiles    { int limit; };

// ---------------------------
// 2. 串接 錯誤類型
// ---------------------------
using Error = std::variant<
  FileNotFoundError, PermissionError, IOError,
  BadFormatError, MemoryError, TooManyOpenFiles
>;

// 建立 Visitor helper
template<class... Ts>
struct Overloaded : Ts... { using Ts::operator()...; };
template<class... Ts>
Overloaded(Ts...) -> Overloaded<Ts...>;

// ---------------------------
// 1. 建立“被測”對象
// ---------------------------
namespace demo 
{
	// 縮寫
	namespace fs = std::filesystem;

	// 功能: 讀取所有檔案 
	// 若檔名含 "PERM_DENIED" 則 PermissionError；
	// 若檔案內容含 "TRIGGER_IO_ERROR" → IOError)
	[[nodiscard]] std::expected<std::string, Error>	ReadAll(const fs::path& p) 
	{
		// 取得檔案位置+檔名
		const auto fname = p.filename().string();
	
		// 檔案不存在
		if (!fs::exists(p)) 
		  return std::unexpected(FileNotFoundError{p.string()});
		
		// 搜尋 "PERM_DENIED"
		if (fname.find("PERM_DENIED") != std::string::npos) 
		  return std::unexpected(PermissionError{p.string()}); // 模擬

		// 開啟檔案
		std::ifstream fin(p, std::ios::binary);
		
		// 無法開啟
		if (!fin.is_open()) 
		  // 無法區分細項，就以一般 I/O 失敗處理
		  return std::unexpected(IOError{p.string(), "open"});

		// 取得內容
		std::string content{std::istreambuf_iterator<char>(fin), {}};
		
		// 確認是否到達檔案結尾 & 讀取是否正常
		if (!fin.good() && !fin.eof()) 
		  return std::unexpected(IOError{p.string(), "read"});
		
		// 確認內容是否含 "TRIGGER_IO_ERROR"，若是則表示I/O錯誤
		if (content.find("TRIGGER_IO_ERROR") != std::string::npos) 
		  return std::unexpected(IOError{p.string(), "read (simulated)"});

		// 成功讀取
		return content;
	}

  // 功能: 解析檔案 
  // 內容含 "MALFORMED" ， 則表示 格式有誤
  // 若字數超過 1024 則 MemoryError（模擬 out-of-memory）
  [[nodiscard]] std::expected<std::string, Error> ParseConfig(std::string content) 
  {
    // 確認內容是否含 "MALFORMED"，若是則表示格式有誤
	if (content.find("MALFORMED") != std::string_view::npos) 
      return std::unexpected(BadFormatError{"MALFORMED token", 1});
    
	// 若自數超過 1024 則 MemoryError（模擬 out-of-memory）
    if (content.size() > 1024) 
      return std::unexpected(MemoryError{"simulated out-of-memory"});
    
	// 模擬解析檔案內容
	for(char &c: content)
		c = c == 0? c : c-1;
	
    // 假設 config 是單純數字；不影響本例
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
  [[nodiscard]] std::expected<int, Error>  LoadAndParse(const fs::path& p) 
  {
    return ReadAll(p).and_then(ParseConfig);
  }
}

// ---------------------------
// 建立Google 測試 特徵
// ---------------------------
class ErrorCasesTest : public ::testing::Test 
{
	protected:
		std::filesystem::path dir;
		
		// 創建檔案
		std::filesystem::path make_file(const std::string& name, std::string_view content) 
		{
			// 取得路徑 + 檔名
			auto p = dir / name;
			// 創建檔案
			std::ofstream(p);
			return p;
		}
		
		// 建構子
		void SetUp() override 
		{
			// 建立 Temp 資料夾
			dir = std::filesystem::temp_directory_path() / "err_cases_demo";
			std::error_code ec;
			std::filesystem::create_directories(dir, ec);
			// 清理舊檔
			for (auto& e : std::filesystem::directory_iterator(dir, ec)) 
			  std::filesystem::remove_all(e.path(), ec);
			
		}
		
		// 解構子
		void TearDown() override 
		{
			std::error_code ec;
			std::filesystem::remove_all(dir, ec);
		}
};

// ---------------------------
// 建立測試場景
// ---------------------------

// 1. 嘗試觸發 FileNotFoundError
TEST_F(ErrorCasesTest, FileNotFound) {
  auto path = dir / "not_exists.json"; // 保證不存在
  auto r = demo::LoadAndParse(path);

  // 使用 variant + visit 檢查具體錯誤型別
  std::visit(Overloaded{
    [&](const FileNotFoundError& e) {EXPECT_EQ(e.path, path.string());},
    [&](const auto&) {ADD_FAILURE() << "Expected FileNotFoundError";}
    }, r.error());
}

// 2. 嘗試觸發 PermissionError
TEST_F(ErrorCasesTest, PermissionError_Read) 
{
  // 檔名帶 PERM_DENIED → 在 ReadAll() 模擬成 PermissionError
  auto path = make_file("PERM_DENIED.json", "whatever");
  auto r = demo::LoadAndParse(path);


  std::visit(Overloaded{
    [&](const PermissionError& e) {EXPECT_EQ(e.path, path.string());},
    [&](const auto&) {ADD_FAILURE() << "Expected PermissionError";}
    }, r.error());
}
// 3. 嘗試觸發 I/O 讀取錯
TEST_F(ErrorCasesTest, IOError_Read) {
  // 內容含 TRIGGER_IO_ERROR 觸發 I/O 讀取錯
  auto path = make_file("io.json", "TRIGGER_IO_ERROR"); 
  auto r = demo::LoadAndParse(path);


  std::visit(Overloaded{
    [&](const IOError& e) {EXPECT_EQ(e.path, path.string()); EXPECT_NE(e.op.find("read"), std::string::npos);},
    [&](const auto&) {ADD_FAILURE() << "Expected IOError";}
    }, r.error());
}

// 4. 嘗試觸發 BadFormat
TEST_F(ErrorCasesTest, BadFormat) 
{
  // 寫入 MALFORMED 觸發 BadFormat
  auto path = make_file("bad.json", "MALFORMED: token here");
  auto r = demo::LoadAndParse(path);


  std::visit(Overloaded{
    [&](const BadFormatError& e) {EXPECT_EQ(e.reason, "MALFORMED token"); EXPECT_EQ(e.line, 1);},
    [&](const auto&) {ADD_FAILURE() << "Expected BadFormatError";}
    }, r.error());
}

// 5. 嘗試觸發 TooManyOpenFiles
TEST_F(ErrorCasesTest, TooManyOpenFiles) {
  // 直接呼叫模擬函式
  auto r = demo::SimulateOpenMany(/*count*/4096, /*limit*/1024);

  std::visit(Overloaded{
    [&](const TooManyOpenFiles& e) {EXPECT_EQ(e.limit, 1024);},
    [&](const auto&) {ADD_FAILURE() << "Expected TooManyOpenFiles";}
    }, r.error());
}

// 進行全測試
int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
