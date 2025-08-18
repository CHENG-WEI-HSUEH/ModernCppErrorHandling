// 這是「Include Guard」開頭：避免此標頭被同一個編譯單位重複包含造成重複定義
#ifndef CONFIG_H
// 與上方成對：定義一個巨集，代表此檔已被包含
#define CONFIG_H

// 引入 C++23 的 std::expected / std::unexpected
#include <expected>
// 檔案輸入輸出（ifstream/ofstream）
#include <fstream>
// 標準輸出入串流（cout/cerr）
#include <iostream>
// 字串串流，方便把整個檔案讀進字串
#include <sstream>
// std::string 型別
#include <string>
// std::variant / std::visit 等列舉型別聯合
#include <variant>

/*
PART I - 定義錯誤類型
PART II - 定義錯誤變數
PART III - 宣告 LoadConfig & ValidateData & ProcessData & handle_pipeline_result
*/

// 開始命名空間：將相關結構與函式封裝
namespace config 
{
	/*==============================1. 定義錯誤類型=====================================*/

	// 定義「讀設定檔錯誤」的錯誤型別，攜帶失敗的檔名
	struct ConfigReadError 
	{
		// 發生讀取錯誤的檔名
		std::string filename;
	};

	// 定義「解析設定檔錯誤」的錯誤型別，攜帶出錯的內容與行號
	struct ConfigParseError 
	{
		// 出錯的那一行原始內容（或簡要片段）
		std::string line_content;
		// 出錯的行號（以 1 起算）
		int line_number;
	};

	// 定義「驗證資料錯誤」的錯誤型別，指出錯誤欄位與其不合法值
	struct ValidationError 
	{
		// 出錯的欄位名稱
		std::string field_name;
		// 不合法的值或說明
		std::string invalid_value;
	};

	// 定義「處理階段錯誤」的錯誤型別，描述任務名稱與細節
	struct ProcessingError 
	{
		// 出錯的任務或步驟名稱
		std::string task_name;
		// 錯誤細節
		std::string details;
	};

	/*==============================2. 定義錯誤變數=====================================*/

	// 將所有錯誤型別聚合成單一 variant，供整條管線的 std::expected 使用
	using PipelineError = std::variant<ConfigReadError,
									   ConfigParseError,
									   ValidationError,
									   ProcessingError>;

	// 建立一個泛型小工具 Overloaded：把多個 Lambda 的 operator() 聚合，方便 std::visit 使用
	template<typename... Ts>
	struct Overloaded : Ts... 
	{
		// 將各個基底（各個 Lambda）的 operator() 帶入此型別
		using Ts::operator()...;
	};

	// 定義「原始設定」資料結構，作為 LoadConfig 的成功結果
	struct Config 
	{
		// 讀入的純文字設定內容
		std::string data;
	};

	// 定義「驗證過的資料」結構，作為 ValidateData 的成功結果
	struct ValidatedData 
	{
		// 為了示範，透過前綴字樣標示已驗證
		std::string processed_data;
	};

	// 定義「最終處理結果」結構，作為 ProcessData 的成功結果
	struct Result 
	{
		// 示範用的結果碼（例如處理後字串長度）
		int final_result_code;
	};

	/*=== 3. 宣告 LoadConfig & ValidateData & ProcessData & handle_pipeline_result ====*/

	// 函式原型宣告：讀設定檔（成功 Config、失敗 PipelineError）
	[[nodiscard]] std::expected<Config,        PipelineError> LoadConfig   (const std::string& filename);
	// 函式原型宣告：驗證資料（成功 ValidatedData、失敗 PipelineError）
	[[nodiscard]] std::expected<ValidatedData, PipelineError> ValidateData (const Config& config);
	// 函式原型宣告：處理資料（成功 Result、失敗 PipelineError）
	[[nodiscard]] std::expected<Result,        PipelineError> ProcessData  (const ValidatedData& data);
// 結束命名空間
}





#endif
