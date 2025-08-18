// 這是「Include Guard」開頭：避免此標頭被同一個編譯單位重複包含造成重複定義
#ifndef CONFIG_PROCESSING_UTILS_H
// 與上方成對：定義一個巨集，代表此檔已被包含
#define CONFIG_PROCESSING_UTILS_H

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

// 宣告一個命名空間，將所有與「設定處理」相關的型別與函式包起來
namespace config 
{

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

	// 這行是 C++17 的類型推導輔助（CTAD），你刻意註解掉以避免在 C++20 模式下可能的相容性議題
	// template<class... Ts>
	// Overloaded(Ts...) -> Overloaded<Ts...>;

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

	// [[nodiscard]]：提醒呼叫端不得忽略回傳；回傳成功為 Config，失敗為 PipelineError
	[[nodiscard]] std::expected<Config, PipelineError> LoadConfig(const std::string& filename) 
	{
		// 嘗試開啟檔案（唯讀）
		std::ifstream file(filename);
		// 若無法成功開啟，立刻回傳 std::unexpected，包裝成 ConfigReadError
		if (!file.is_open()) 
		{
			// 除錯輸出：指出無法開啟的檔名
			std::cerr << "DEBUG: LoadConfig failed to open " << filename << std::endl;
			// 使用 C++23 的 std::unexpected 傳遞錯誤，攜帶檔名
			return std::unexpected(ConfigReadError{filename});
		}
		// 準備一個字串串流，將整個檔案內容讀入
		std::stringstream buffer;
		// 將檔案緩衝（rdbuf）送進字串串流
		buffer << file.rdbuf();
		// 取出完整字串內容
		std::string content = buffer.str();

		// 示範用途：若內容為空，或包含 "malformed" 字樣，視為解析錯誤
		if (content.empty() || content.find("malformed") != std::string::npos) 
		{
			// 除錯輸出：提示偵測到格式不正確
			std::cerr << "DEBUG: LoadConfig detected malformed config in " << filename << std::endl;
			// 回傳解析錯誤，這裡行號固定示範為 1，行內容以 "malformed" 字樣代表
			return std::unexpected(ConfigParseError{"malformed", 1});
		}
		// 除錯輸出：讀檔成功
		std::cout << "DEBUG: Config loaded successfully from " << filename << std::endl;
		// 回傳成功：包裝為 Config
		return Config{content};
	}

	// [[nodiscard]]：提醒呼叫端不得忽略回傳；成功為 ValidatedData，失敗為 PipelineError
	[[nodiscard]] std::expected<ValidatedData, PipelineError> ValidateData(const Config& config) 
	{
		// 示範用途：若設定內容包含 "invalid_field" 字樣，視為驗證失敗
		if (config.data.find("invalid_field") != std::string::npos) 
		{
			// 除錯輸出：提示偵測到不合法欄位
			std::cerr << "DEBUG: ValidateData detected invalid field." << std::endl;
			// 回傳驗證錯誤，攜帶欄位名稱與不合法值描述
			return std::unexpected(ValidationError{"invalid_field", "contains disallowed value"});
		}
		// 除錯輸出：驗證成功
		std::cout << "DEBUG: Data validated successfully." << std::endl;
		// 回傳成功：把原內容前綴 "Validated: "，以示意已通過驗證
		return ValidatedData{"Validated: " + config.data};
	}

	// [[nodiscard]]：提醒呼叫端不得忽略回傳；成功為 Result，失敗為 PipelineError
	[[nodiscard]] std::expected<Result, PipelineError> ProcessData(const ValidatedData& data) 
	{
		// 示範用途：若驗後字串長度小於 10，視為處理失敗（資料過短）
		if (data.processed_data.length() < 10) 
		{
			// 除錯輸出：提示資料過短
			std::cerr << "DEBUG: ProcessData detected data too short." << std::endl;
			// 回傳處理錯誤，指名任務名稱與詳細說明
			return std::unexpected(ProcessingError{"Data Processing", "Input data too short for task"});
		}
		// 除錯輸出：處理成功
		std::cout << "DEBUG: Data processed successfully." << std::endl;
		// 回傳成功：結果碼以處理後字串長度為例
		return Result{static_cast<int>(data.processed_data.length())};
	}

// 結束命名空間（下行註解的 "namepsace" 為小拼字；應為 "namespace"）
} 


#endif
