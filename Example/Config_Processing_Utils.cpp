// 引入對應的宣告標頭
#include "Config_Processing_Utils.h"
// 引入檔案 I/O
#include <fstream>
// 引入標準輸出入（除錯訊息）
#include <iostream>
// 引入字串串流（整檔讀入）
#include <sstream>

// 進入命名空間
namespace config 
{

	// 加上 [[nodiscard]]：提醒呼叫端不可忽略回傳結果
	[[nodiscard]] std::expected<Config, PipelineError>	LoadConfig(const std::string& filename) 
	{
		// 嘗試開啟檔案（唯讀）
		std::ifstream file(filename);
		// 若檔案開啟失敗，回傳讀檔錯誤
		if (!file.is_open()) {
			// 印出除錯訊息（非必要，但有助示範）
			std::cerr << "DEBUG: LoadConfig failed to open " << filename << std::endl;
			// 以 std::unexpected 包裝 ConfigReadError 作為失敗回傳
			return std::unexpected(ConfigReadError{filename});
		}

		// 準備字串串流，將檔案完整讀入
		std::stringstream buffer;
		// 將檔案緩衝區寫入字串串流
		buffer << file.rdbuf();
		// 取得整份內容
		std::string content = buffer.str();

		// 示範條件：空檔或包含關鍵字 "malformed" 視為解析錯誤
		if (content.empty() || content.find("malformed") != std::string::npos) 
		{
			// 除錯訊息：指出解析不合法
			std::cerr << "DEBUG: LoadConfig detected malformed config in " << filename << std::endl;
			// 回傳解析錯誤（行號固定示範為 1）
			return std::unexpected(ConfigParseError{"malformed", 1});
		}

		// 除錯訊息：讀檔、基本檢查皆成功
		std::cout << "DEBUG: Config loaded successfully from " << filename << std::endl;
		// 回傳成功資料（Config）
		return Config{content};
	}

	// 加上 [[nodiscard]]：提醒呼叫端不可忽略回傳結果
	[[nodiscard]] std::expected<ValidatedData, PipelineError>
	ValidateData(const Config& config) {
		// 示範條件：若包含 "invalid_field" 視為驗證失敗
		if (config.data.find("invalid_field") != std::string::npos) 
		{
			// 除錯訊息：驗證不通過
			std::cerr << "DEBUG: ValidateData detected invalid field." << std::endl;
			// 回傳驗證錯誤與欄位資訊
			return std::unexpected(ValidationError{"invalid_field", "contains disallowed value"});
		}

		// 除錯訊息：驗證通過
		std::cout << "DEBUG: Data validated successfully." << std::endl;
		// 回傳成功資料（附上前綴表示已驗證）
		return ValidatedData{"Validated: " + config.data};
	}

	// 加上 [[nodiscard]]：提醒呼叫端不可忽略回傳結果
	[[nodiscard]] std::expected<Result, PipelineError>
	ProcessData(const ValidatedData& data) 
	{
		// 示範條件：處理前的資料長度小於 10 視為處理失敗
		if (data.processed_data.length() < 10) 
		{
			// 除錯訊息：處理失敗，資料太短
			std::cerr << "DEBUG: ProcessData detected data too short." << std::endl;
			// 回傳處理階段錯誤（任務名稱＋說明）
			return std::unexpected(ProcessingError{"Data Processing", "Input data too short for task"});
		}

		// 除錯訊息：處理成功
		std::cout << "DEBUG: Data processed successfully." << std::endl;
		// 回傳結果（此處以字串長度當作結果碼）
		return Result{static_cast<int>(data.processed_data.length())};
	}

// 結束命名空間
} 
