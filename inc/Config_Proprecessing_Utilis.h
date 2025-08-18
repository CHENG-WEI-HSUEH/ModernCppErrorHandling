#ifndef CONFIG_PROCESSING_UTILS_H
#define CONFIG_PROCESSING_UTILS_H

#include <expected>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <variant>

namespace config {

struct ConfigReadError {
    std::string filename;
};

struct ConfigParseError {
    std::string line_content;
    int line_number;
};

struct ValidationError {
    std::string field_name;
    std::string invalid_value;
};

struct ProcessingError {
    std::string task_name;
    std::string details;
};

using PipelineError = std::variant<ConfigReadError,
                                   ConfigParseError,
                                   ValidationError,
                                   ProcessingError>;

template<typename... Ts>
struct Overloaded : Ts... {
    using Ts::operator()...;
};

// Commented out to avoid issues with C++20
// template<class... Ts>
// Overloaded(Ts...) -> Overloaded<Ts...>;

struct Config {
    std::string data;
};

struct ValidatedData {
    std::string processed_data;
};

struct Result {
    int final_result_code;
};

[[nodiscard]] std::expected<Config, PipelineError> LoadConfig(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "DEBUG: LoadConfig failed to open " << filename << std::endl;
        return std::unexpected(ConfigReadError{filename});
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();

    // Simulate a parse error for empty config or specific content
    if (content.empty() || content.find("malformed") != std::string::npos) {
        std::cerr << "DEBUG: LoadConfig detected malformed config in " << filename << std::endl;
        return std::unexpected(ConfigParseError{"malformed", 1});
    }
    std::cout << "DEBUG: Config loaded successfully from " << filename << std::endl;
    return Config{content};
}

[[nodiscard]] std::expected<ValidatedData, PipelineError> ValidateData(const Config& config) {
    // Simulate a validation error
    if (config.data.find("invalid_field") != std::string::npos) {
        std::cerr << "DEBUG: ValidateData detected invalid field." << std::endl;
        return std::unexpected(ValidationError{"invalid_field", "contains disallowed value"});
    }
    std::cout << "DEBUG: Data validated successfully." << std::endl;
    return ValidatedData{"Validated: " + config.data};
}

[[nodiscard]] std::expected<Result, PipelineError> ProcessData(const ValidatedData& data) {
    // Simulate a processing error
    if (data.processed_data.length() < 10) {
        std::cerr << "DEBUG: ProcessData detected data too short." << std::endl;
        return std::unexpected(ProcessingError{"Data Processing", "Input data too short for task"});
    }
    std::cout << "DEBUG: Data processed successfully." << std::endl;
    return Result{static_cast<int>(data.processed_data.length())};
}

} // namepsace config

#endif // CONFIG_PROCESSING_UTILS_H
