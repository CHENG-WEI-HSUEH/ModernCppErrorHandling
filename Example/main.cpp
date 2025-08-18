// 引入我們的工具標頭（宣告）
#include "Config_Processing_Utils.h"
// 引入 <cstdio> 以使用 std::remove 刪除檔案
#include <cstdio>
// 引入 <fstream> 以使用 std::ofstream 建立示範檔案
#include <fstream>
// 引入標準輸出入（顯示結果）
#include <iostream>
// 引入 std::string
#include <string>
// 引入 std::variant（搭配 std::visit）
#include <variant>

// 使用命名空間
using namespace config;

// 宣告一個輔助函式：負責把最終結果（成功或錯誤）輸出到主控台
static void HandlePipelineResult(const std::expected<Result, PipelineError>& r) 
{
    // 若 r 為成功（含有值）
    if (r) 
	{
        // 輸出成功訊息與結果碼
        std::cout << "\nPipeline Succeeded! Final Result Code: " << r->final_result_code << '\n';
        // 回傳結束此函式
        return;
    }
    // 否則為失敗：先輸出前綴訊息
    std::cerr << "\nPipeline Failed! Error details: ";
    // 使用 std::visit 根據實際錯誤型別分派處理
    std::visit(Overloaded{
        // 若為讀檔錯誤
        [](const ConfigReadError& e) 
		{
            // 印出檔名
            std::cerr << "Configuration Read Error: Could not open file '" << e.filename << "'\n";
        },
        // 若為解析錯誤
        [](const ConfigParseError& e) 
		{
            // 印出行號與片段
            std::cerr << "Configuration Parse Error: Malformed content at line "
                      << e.line_number << " (Context: '" << e.line_content << "')\n";
        },
        // 若為驗證錯誤
        [](const ValidationError& e) 
		{
            // 印出欄位與不合法值
            std::cerr << "Data Validation Error: Field '" << e.field_name
                      << "' has invalid value '" << e.invalid_value << "'\n";
        },
        // 若為處理錯誤
        [](const ProcessingError& e) 
		{
            // 印出任務名稱與細節
            std::cerr << "Data Processing Error: Task '" << e.task_name
                      << "' failed. Details: " << e.details << '\n';
        }
        // Overloaded 結束
    }, r.error());
}

// 主程式進入點
int main() 
{
    // 情境一：成功案例
    std::cout << "--- Scenario 1: Successful Execution ---" << std::endl;
    // 建立一個內容充分的合法設定檔
    std::ofstream("valid_config.txt") << "valid_data_content";
    // 建立管線：讀檔 → 驗證 → 處理；任何一步錯誤，後續 and_then 不會執行
    auto ok = LoadConfig("valid_config.txt")
            .and_then([](const Config& cfg){ return ValidateData(cfg); })
            .and_then([](const ValidatedData& vd){ return ProcessData(vd); });
    // 輸出此情境的結果
    HandlePipelineResult(ok);

    // 情境二：讀檔錯誤（檔案不存在）
    std::cout << "\n--- Scenario 2: Config Read Error ---" << std::endl;
    // 嘗試讀取不存在的檔案，將直接得到 ConfigReadError
    auto rerr = LoadConfig("non_existent_config.txt")
              .and_then([](const Config& cfg){ return ValidateData(cfg); })
              .and_then([](const ValidatedData& vd){ return ProcessData(vd); });
    // 輸出此情境的結果
    HandlePipelineResult(rerr);

    // 情境三：解析錯誤（內容含 "malformed"）
    std::cout << "\n--- Scenario 3: Config Parse Error ---" << std::endl;
    // 建立一個包含 "malformed" 的檔案
    std::ofstream("malformed_config.txt") << "malformed content";
    // 讀檔後將在 LoadConfig 階段回傳 ConfigParseError
    auto perr = LoadConfig("malformed_config.txt")
              .and_then([](const Config& cfg){ return ValidateData(cfg); })
              .and_then([](const ValidatedData& vd){ return ProcessData(vd); });
    // 輸出此情境的結果
    HandlePipelineResult(perr);

    // 情境四：驗證錯誤（內容含 "invalid_field"）
    std::cout << "\n--- Scenario 4: Validation Error ---" << std::endl;
    // 建立一個在後續驗證會失敗的檔案
    std::ofstream("invalid_data_config.txt") << "valid_data\ninvalid_field";
    // 讀檔成功，但 ValidateData 會回傳 ValidationError
    auto verr = LoadConfig("invalid_data_config.txt")
              .and_then([](const Config& cfg){ return ValidateData(cfg); })
              .and_then([](const ValidatedData& vd){ return ProcessData(vd); });
    // 輸出此情境的結果
    HandlePipelineResult(verr);

    // 情境五：處理錯誤（驗後資料過短）
    std::cout << "\n--- Scenario 5: Processing Error ---" << std::endl;
    // 建立一個字串過短的檔案
    std::ofstream("short_data_config.txt") << "short";
    // 讀檔、驗證通過，但 ProcessData 會因長度不足回錯誤
    auto perr2 = LoadConfig("short_data_config.txt")
               .and_then([](const Config& cfg){ return ValidateData(cfg); })
               .and_then([](const ValidatedData& vd){ return ProcessData(vd); });
    // 輸出此情境的結果
    HandlePipelineResult(perr2);

    // 清理測試檔案（避免殘留）
    std::remove("valid_config.txt");
    std::remove("malformed_config.txt");
    std::remove("invalid_data_config.txt");
    std::remove("short_data_config.txt");
    // 返回 0 代表程式正常結束
    return 0;
}
