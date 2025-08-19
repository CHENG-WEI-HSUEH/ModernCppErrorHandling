\documentclass[12pt,a4paper]{article}
\usepackage[UTF8]{ctex} % 中文支援（建議用 XeLaTeX 編譯）
\usepackage{geometry}
\geometry{margin=1in}
\usepackage{hyperref}
\hypersetup{colorlinks=true, linkcolor=blue, urlcolor=blue}
\usepackage{listings}
\usepackage{xcolor}

\lstdefinelanguage{C++}{
  language=C++,
  morekeywords={concept,requires,consteval,constinit,co_await,co_return,co_yield},
  sensitive=true
}
\lstset{
  language=C++,
  basicstyle=\ttfamily\small,
  columns=fullflexible,
  breaklines=true,
  frame=single,
  showstringspaces=false,
  tabsize=2,
  keywordstyle=\color{blue!70!black}\bfseries,
  commentstyle=\color{green!40!black}\itshape,
  stringstyle=\color{red!60!black}
}

\title{現代 C++ 錯誤處理要點（variant / expected / optional / [[nodiscard]]）}
\author{技術筆記}
\date{\today}

\begin{document}
\maketitle

\section{1.\ using \texttt{PipelineError = std::variant<...>}}
\textbf{目的}：將多種錯誤型別聚合為型別安全的聯合（tagged union）。同一時間只持有其中一種錯誤型別，並保留型別資訊以便處理。

\noindent\textbf{範例}：
\begin{lstlisting}
// 聚合不同錯誤的 payload
struct ReadError  { std::string filename; };
struct ParseError { int line; std::string msg; };

// 以 std::variant 統一錯誤型別
using PipelineError = std::variant<ReadError, ParseError>;
\end{lstlisting}

\section{2.\ Overloaded（合併多個 lambda 供 \texttt{std::visit} 使用）}
\textbf{目的}：用多重繼承把多個 lambda 合併成一個可呼叫物件，方便對 \texttt{variant} 進行「模式比對」式分派。

\noindent\textbf{範例}：
\begin{lstlisting}
template<class... Ts>
struct Overloaded : Ts... { using Ts::operator()...; };
template<class... Ts>
Overloaded(Ts...) -> Overloaded<Ts...>;

std::visit(Overloaded{
  [](const ReadError&  e){ std::cerr << "Read fail: "  << e.filename << "\n"; },
  [](const ParseError& e){ std::cerr << "Parse fail@"  << e.line << ": " << e.msg << "\n"; }
}, someVariant);
\end{lstlisting}

\section{3.\ \texttt{[[nodiscard]]}}
\textbf{目的}：提醒（甚至要求）呼叫端\emph{不要忽略}回傳值，忽略時編譯器會警告。對錯誤處理容器（如 \texttt{expected} / \texttt{optional}）尤其重要。

\noindent\textbf{範例}：
\begin{lstlisting}
[[nodiscard]] bool ok();
[[nodiscard("must check the result")]] std::expected<int, std::string> compute();
\end{lstlisting}

\section{4.\ \texttt{std::expected<T,E>}（C++23）}
\textbf{語義}：要麼是成功的 \texttt{T}，要麼是錯誤的 \texttt{E}。提供單子操作（\texttt{and\_then}/\texttt{transform}/\texttt{or\_else}）讓流程可組合、可讀。

\noindent\textbf{常見介面}：\texttt{has\_value()}、\texttt{value()}、\texttt{error()}、\texttt{and\_then()}、\texttt{transform()}、\texttt{or\_else()}。

\noindent\textbf{範例}（讀檔字串 $\rightarrow$ 解析整數 $\rightarrow$ 倍增）：
\begin{lstlisting}
#include <expected>
#include <fstream>
#include <string>

struct ReadError { std::string filename; };
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

  if (v) { /* 使用 *v */ } else { /* 根據 v.error() 分派 */ }
}
\end{lstlisting}

\section{5.\ \texttt{std::variant}}
\textbf{語義}：在多個候選型別之間擇一持有；搭配 \texttt{std::visit} 進行型別安全的動態分派。

\noindent\textbf{常見介面}：\texttt{std::holds\_alternative<T>}、\texttt{std::get<T>}、\texttt{std::get\_if<T>}、\texttt{index()}、\texttt{valueless\_by\_exception()}。

\noindent\textbf{範例}（錯誤列印）：
\begin{lstlisting}
#include <variant>
#include <iostream>

struct ReadError { std::string filename; };
struct ParseError { int line; std::string msg; };
using Error = std::variant<ReadError, ParseError>;

template<class... Ts> struct Overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> Overloaded(Ts...) -> Overloaded<Ts...>;

void print_error(const Error& e) {
  std::visit(Overloaded{
    [](const ReadError&  r){ std::cerr << "ReadError: "  << r.filename << "\n"; },
    [](const ParseError& p){ std::cerr << "ParseError@"  << p.line << ": " << p.msg << "\n"; }
  }, e);
}
\end{lstlisting}

\section{6.\ \texttt{std::optional<T>}}
\textbf{語義}：可能有一個 \texttt{T} 的值，也可能沒有；\textbf{不}攜帶錯誤資訊。適合「不存在是合理狀態」且不需說明原因的場合。C++23 也加入 \texttt{and\_then}/\texttt{transform}/\texttt{or\_else}。

\noindent\textbf{範例}（查表）：
\begin{lstlisting}
#include <optional>
#include <string>
#include <unordered_map>

std::optional<int> get_price(const std::string& sku) {
  static const std::unordered_map<std::string,int> db = {{"A",100},{"B",200}};
  if (auto it = db.find(sku); it != db.end()) return it->second;
  return std::nullopt; // 找不到不是錯，只是沒有
}

int main() {
  int price = get_price("C").value_or(0); // 沒有就回預設 0
}
\end{lstlisting}

\section{綜合示例：三段管線與四種錯誤}
\noindent\textbf{說明}：使用 \texttt{std::expected<T,E>} 串接 \texttt{LoadConfig} $\rightarrow$ \texttt{ValidateData} $\rightarrow$ \texttt{ProcessData}，\texttt{E} 是 \texttt{std::variant} 聚合的多類錯誤，並用 \texttt{Overloaded} 配合 \texttt{std::visit} 分派。

\begin{lstlisting}
// g++ -std=gnu++23 -O2 demo.cpp -o demo && ./demo
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

template<class... Ts> struct Overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> Overloaded(Ts...) -> Overloaded<Ts...>;

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
\end{lstlisting}

\section{選擇建議}
\begin{itemize}
  \item \textbf{optional}：只有「有/無」，不需錯誤上下文。
  \item \textbf{variant}：值可在多型別間擇一；適合\emph{表達多種錯誤類別}或資料變體。
  \item \textbf{expected}：\emph{成功或錯誤}，需要攜帶錯誤資訊且想避免例外時的首選；錯誤型別可用 \texttt{variant} 聚合。
  \item \textbf{[[nodiscard]]}：回傳這些容器的 API 建議加註，避免調用方忽略。
\end{itemize}

\end{document}
