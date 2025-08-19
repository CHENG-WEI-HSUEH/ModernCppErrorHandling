# 現代 C++ 錯誤處理要點（`variant` / `expected` / `optional` / `[[nodiscard]]`）

> 環境建議：GCC 13+ 或 Clang 16+，編譯選項 `-std=c++23`（因為使用 `std::expected`）。

---

## 1) `using PipelineError = std::variant<...>`

**用途**：把「多種錯誤型別」聚合成**單一錯誤容器**。`std::variant` 是 C++17 的型別安全聯合（tagged union），同一時間只持有其中一種型別的值，並保留型別資訊。

**範例**
```cpp
#include <variant>
#include <string>

struct ReadError  { std::string filename; };
struct ParseError { int line; std::string msg; };

using PipelineError = std::variant<ReadError, ParseError>;
```

---

## 2) `Overloaded`（合併多個 lambda 供 `std::visit` 使用）

**用途**：把多個 lambda 經由多重繼承合併成**一個**訪客，讓 `std::visit` 針對 `variant` 目前持有的型別做模式比對式分派。

**範例**
```cpp
template<class... Ts>
struct Overloaded : Ts... { using Ts::operator()...; }

// C++17 類型推導（可選）
// template<class... Ts> Overloaded(Ts...) -> Overloaded<Ts...>;

std::variant<ReadError, ParseError> v = ReadError{"config.json"};

std::visit(Overloaded{
  [](const ReadError&  e){ std::cerr << "Read fail: "  << e.filename << "\n"; },
  [](const ParseError& e){ std::cerr << "Parse fail@"  << e.line << ": " << e.msg << "\n"; }
}, v);
```

---

## 3) `[[nodiscard]]`

**用途**：標註函式（或型別），**提醒不要忽略回傳值**；忽略時編譯器會警告。對錯誤處理容器（`expected` / `optional`）尤其重要。

**範例**
```cpp
[[nodiscard]] bool ok();

[[nodiscard("must check result")]]
std::expected<int, std::string> compute();
```

---

## 4) `std::expected<T,E>`（C++23）

**語義**：**要嘛是成功的 `T`，要嘛是錯誤的 `E`**。提供單子操作（`and_then` / `transform` / `or_else` / `transform_error`），讓流程**可組合**且**可讀**；比起 exceptions，更顯式、可預測。

**常見介面**
- `has_value()` / `operator bool()`
- `value()` / `*` / `->`
- `error()`
- `and_then(fn)`、`transform(fn)`、`or_else(fn)`、`transform_error(fn)`

**範例：讀檔→解析整數→倍增**
```cpp
#include <expected>
#include <fstream>
#include <string>
#include <variant>

struct ReadError  { std::string filename; };
struct ParseError { std::string text; };
using Err = std::variant<ReadError, ParseError>;

[[nodiscard]] std::expected<std::string, Err>
read_all(const std::string& path) {
  std::ifstream fin(path);
  if (!fin) return std::unexpected(ReadError{path});
  return std::string{std::istreambuf_iterator<char>(fin), {}};
}

[[nodiscard]] std::expected<int, Err>
parse_int(std::string s) {
  try { return std::stoi(s); }
  catch (...) { return std::unexpected(ParseError{s}); }
}

int main() {
  auto v = read_all("num.txt")
         .and_then(parse_int)
         .transform([](int x){ return x * 2; });

  if (v) { /* 使用 *v */ } else { /* 根據 v.error() 做分派 */ }
}
```

---

## 5) `std::variant`（C++17）

**語義**：在多種候選型別間擇一持有；搭配 `std::visit` 進行型別安全的動態分派。

**常見介面**
- `std::holds_alternative<T>(v)`
- `std::get<T>(v)` / `std::get_if<T>(&v)`
- `v.index()` / `v.valueless_by_exception()`
- `std::monostate`（需要「空型別」時）

**範例：錯誤列印**
```cpp
#include <variant>
#include <iostream>
#include <string>

struct ReadError  { std::string filename; };
struct ParseError { int line; std::string msg; };
using Error = std::variant<ReadError, ParseError>;

template<class... Ts> struct Overloaded : Ts... { using Ts::operator()...; }
// template<class... Ts> Overloaded(Ts...) -> Overloaded<Ts...>;

void print_error(const Error& e) {
  std::visit(Overloaded{
    [](const ReadError&  r){ std::cerr << "ReadError: "  << r.filename << "\n"; },
    [](const ParseError& p){ std::cerr << "ParseError@"  << p.line << ": " << p.msg << "\n"; }
  }, e);
}
```

---

## 6) `std::optional<T>`（C++17；C++23 擴充單子操作）

**語義**：**要嘛有一個 `T`，要嘛沒有**；不包含錯誤資訊。適合「不存在是正常情況」且不需要說明為何不存在的場景。C++23 也加入 `and_then` / `transform` / `or_else`。

**範例：查表**
```cpp
#include <optional>
#include <string>
#include <unordered_map>

std::optional<int> get_price(const std::string& sku) {
  static const std::unordered_map<std::string,int> db = {{"A",100},{"B",200}};
  if (auto it = db.find(sku); it != db.end()) return it->second;
  return std::nullopt; // 找不到不是錯，只是沒有
}

int main() {
  int price = get_price("C").value_or(0); // 沒有就回預設值 0
}
```

---

## 綜合示例：三段管線與四種錯誤（可編譯）

> 編譯：`g++ -std=gnu++23 -O2 demo.cpp -o demo && ./demo`

```cpp
#include <expected>
#include <variant>
#include <string>
#include <fstream>
#include <iostream>

struct ConfigReadError { std::string filename; };
struct ConfigParseError{ std::string msg; int line; };
struct ValidationError { std::string field; std::string why; };
struct ProcessingError { std::string task; std::string detail; };

using PipelineError = std::variant<ConfigReadError, ConfigParseError,
                                   ValidationError, ProcessingError>;

template<class... Ts> struct Overloaded : Ts... { using Ts::operator()...; }
// template<class... Ts> Overloaded(Ts...) -> Overloaded<Ts...>;

struct Config        { std::string data; };
struct ValidatedData { std::string processed; };
struct Result        { int code; };

[[nodiscard]] std::expected<Config, PipelineError>
LoadConfig(const std::string& path) {
  std::ifstream fin(path);
  if (!fin) return std::unexpected(ConfigReadError{path});
  std::string s{std::istreambuf_iterator<char>(fin), {}};
  if (s.empty() || s.find("malformed") != std::string::npos)
    return std::unexpected(ConfigParseError{"malformed", 1});
  return Config{s};
}

[[nodiscard]] std::expected<ValidatedData, PipelineError>
ValidateData(const Config& c) {
  if (c.data.find("invalid_field") != std::string::npos)
    return std::unexpected(ValidationError{"invalid_field", "disallowed"});
  return ValidatedData{"Validated: " + c.data};
}

[[nodiscard]] std::expected<Result, PipelineError>
ProcessData(const ValidatedData& v) {
  if (v.processed.size() < 10)
    return std::unexpected(ProcessingError{"Data Processing", "too short"});
  return Result{static_cast<int>(v.processed.size())};
}

int main() {
  std::ofstream("ok.txt") << "hello world";
  auto r = LoadConfig("ok.txt")
         .and_then(ValidateData)
         .and_then(ProcessData);

  if (r) {
    std::cout << "OK, code=" << r->code << "\n";
  } else {
    std::visit(Overloaded{
      [](const ConfigReadError& e){ std::cerr<<"read fail: "<<e.filename<<"\n"; },
      [](const ConfigParseError& e){ std::cerr<<"parse fail@"<<e.line<<": "<<e.msg<<"\n"; },
      [](const ValidationError& e){ std::cerr<<"validate fail "<<e.field<<": "<<e.why<<"\n"; },
      [](const ProcessingError& e){ std::cerr<<"process fail "<<e.task<<": "<<e.detail<<"\n"; }
    }, r.error());
  }
}
```

---

## 何時選什麼？

- **`std::optional<T>`**：只有「有/無」，不需錯誤上下文（如 map 查不到）。
- **`std::variant<A,B,...>`**：值可在多型別間擇一；適合表達**多種錯誤類別**或資料變體；搭配 `std::visit`。
- **`std::expected<T,E>`**：**成功或錯誤**的容器，當你需要**攜帶錯誤資訊**、又想**避免例外**時的首選；`E` 常常就是一個 `std::variant`。
- **`[[nodiscard]]`**：回傳上述容器的 API 建議加註，避免調用方忽略結果。
