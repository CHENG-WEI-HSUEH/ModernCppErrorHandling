#include <gtest/gtest.h>
#include <Config.h>
#include <expected>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <variant>

typedef struct _Vars
{
	string filename;
	
}Vars;
const Vars AcsVars(Vars *input)
{
	if(input != nullptr)	static Vars output = input;
	return output;
}
// ---------------------------
// 1. 載入測試對象特徵
// ---------------------------
class ErrorCasesTest : public ::testing::Test {
protected:
	
	const Vars VarsInfo;
	
	//創建檔案
	std::filesystem::path make_file(const std::filesystem::path& filepath, std::string filename) 
	{
		//結合檔案路徑
		auto p = filepath / filename;
		//創建檔案
		std::ofstream(p);
		return p;
	}
	
	//Google Test 初始化
	void SetUp() override 
	{
		// 取得傳入參數
		VarsInfo = AcsVars(nullptr);
		
		// 創建臨時資料夾
		dir = std::filesystem::temp_directory_path() / "Temp";
		std::error_code ec;
		std::filesystem::create_directories(dir, ec);
	}
	// Google Test 解構
	void TearDown() override 
	{
		std::error_code ec;
		std::filesystem::remove_all(dir, ec);
	}
};

// ---------------------------
// 2. 加入測試場景
// ---------------------------

// 場景一 - 嘗試載入檔案
TEST_F(ErrorCasesTest, FileNotFound) {

	// 創建"不存在的路徑"
	auto path = dir / "not_exists.json"; 
	
	// 建立測試流程 : 讀檔 -> 資料驗證(是否包含 "invalid_field") -> 檢查資料長度
    auto PP1_Res = LoadConfig(path)
            .and_then([](const Config& cfg){ return ValidateData(cfg); })
            .and_then([](const ValidatedData& vd){ return ProcessData(vd); });	
	
	// 成功載入檔案 (顯示結果)
	if(PP1_Res)
	{
        std::cout << "\nPipeline Succeeded! Final Result Code: " << r->final_result_code << '\n';
		return;
	}
	
	// 使用 variant + visit 檢查具體錯誤型別
	std::visit(Overloaded
	{
		[](const FileNotFoundError& e) 
		{
		  EXPECT_EQ(e.path, path.string());
		},
		[](const auto&) 
		{
		  ADD_FAILURE() << "Expected FileNotFoundError";
		}
	}, res.error());
}


// 3. 開始測試所有場景
int main(int argc, char** argv) 
{
	//抓取 User 傳入參數
	Vars fileinfo = {argv[1]};
	AcsVars(&fileinfo);
  
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
