# Google Test 常用函式／巨集速查（Quick Reference）— 表格版（g++ 版）

> 先用表格列出**介面、用途、輸入、效果/輸出**；後面附上**逐項範例**可直接以 `g++` 編譯。  
> 建議環境：Ubuntu / WSL。安裝方式可用套件或將 GoogleTest 以子模組加入並自行編譯。編譯連結時請記得 `-lgtest -lpthread`。

---

## 1) 定義測試 / 測試治具

| 介面 | 類別 | 用途 | 輸入參數 | 效果/輸出 | 備註 |
|---|---|---|---|---|---|
| `TEST(Suite, Name)` | 定義 | 宣告單一測試 | `Suite`: 測試套件名；`Name`: 測試名 | 產生一個測試 | 不需 fixture |
| `TEST_F(Fixture, Name)` | 定義 | 使用測試治具 | `Fixture`: 類別名；`Name`: 測試名 | 產生一個依賴 fixture 的測試 | 可用 `SetUp`/`TearDown` |
| `TEST_P(FixtureWithParam, Name)` | 定義 | 參數化測試 | `FixtureWithParam<T>`；`Name` | 產生可重複的測試模板 | 搭配 `INSTANTIATE_TEST_SUITE_P` |
| `INSTANTIATE_TEST_SUITE_P(Prefix, Fixture, Values...)` | 定義 | 指定參數組 | `Prefix`: 前綴名；`Fixture`；`Values` | 展開多個具體測試 | 支援 `Values`, `Range`, `Combine` |
| `::testing::InitGoogleTest(&argc, argv)` | 初始化 | 解析 gtest 旗標 | `argc/argv` | 設定測試執行環境 | main 內呼叫 |
| `RUN_ALL_TESTS()` | 執行 | 執行全部測試 | — | 回傳 `0`/`1` | main 的 return |

---

## 2) 一般斷言（`EXPECT_*` 非致命；`ASSERT_*` 致命、會中止當前測試）

| 介面 | 類別 | 用途 | 輸入參數 | 效果/輸出 | 備註 |
|---|---|---|---|---|---|
| `EXPECT_TRUE(x)` / `EXPECT_FALSE(x)` | 布林 | 期望為真/假 | `x`: 可轉為 bool | 非致命失敗 | `ASSERT_*` 版本會中止 |
| `EXPECT_EQ(a,b)` / `EXPECT_NE(a,b)` | 相等 | 相等/不等 | `a`,`b` 可比較 | 非致命失敗 | 顯示左右值 |
| `EXPECT_LT/LE/GT/GE(a,b)` | 比較 | 小於/小於等於/大於/大於等於 | `a`,`b` | 非致命失敗 | 數值/可比較型別 |

---

## 3) 字串與浮點

| 介面 | 類別 | 用途 | 輸入參數 | 效果/輸出 | 備註 |
|---|---|---|---|---|---|
| `EXPECT_STREQ/STRNE(s1,s2)` | 字串 | C 字串內容相等/不等 | `const char*` | 非致命失敗 | 比**內容**不是位址 |
| `EXPECT_STRCASEEQ/STRCASENE(s1,s2)` | 字串 | 忽略大小寫 | 同上 | 非致命失敗 | 針對 C 字串 |
| `EXPECT_FLOAT_EQ/DOUBLE_EQ(a,b)` | 浮點 | 以 ULP 判等 | `float/double` | 非致命失敗 | 自動誤差界 |
| `EXPECT_NEAR(a,b,eps)` | 浮點 | 絕對誤差 | `a`,`b`,`eps` | 非致命失敗 | 自訂容忍度 |

---

## 4) 例外

| 介面 | 類別 | 用途 | 輸入參數 | 效果/輸出 | 備註 |
|---|---|---|---|---|---|
| `EXPECT_THROW(stmt, Ex)` | 例外 | 期望丟 `Ex` | `stmt`、例外型別 | 非致命失敗 | `ASSERT_THROW` 致命 |
| `EXPECT_NO_THROW(stmt)` | 例外 | 期望不丟例外 | `stmt` | 非致命失敗 | — |
| `EXPECT_ANY_THROW(stmt)` | 例外 | 期望丟任意例外 | `stmt` | 非致命失敗 | — |

---

## 5) 控制流程

| 介面 | 類別 | 用途 | 輸入參數 | 效果/輸出 | 備註 |
|---|---|---|---|---|---|
| `GTEST_SKIP()` | 跳過 | 執行期跳過測試 | 可串 `<<` 訊息 | 當前測試標記為 skipped | 不中止其他測試 |
| `SUCCEED()` | 成功 | 顯式標記成功 | 可串 `<<` 訊息 | 新增一個成功訊息 | 偶爾用於記錄 |
| `ADD_FAILURE()` | 失敗 | 強制失敗（非致命） | 可串 `<<` | 非致命失敗 | `GTEST_FAIL()` 為致命 |

---

## 6) Matchers（`EXPECT_THAT(actual, matcher)`）

| 介面 | 類別 | 用途 | 輸入參數 | 效果/輸出 | 備註 |
|---|---|---|---|---|---|
| `EXPECT_THAT(v, ElementsAre(...))` | 容器 | 逐一比對 | 容器、元素序列 | 非致命失敗 | 需 `using ::testing::ElementsAre;` |
| `EXPECT_THAT(v, Contains(x))` | 容器 | 包含元素 | 容器、元素 | 非致命失敗 | — |
| `HasSubstr/StartsWith/EndsWith` | 字串 | 子字串/開頭/結尾 | `std::string` | 非致命失敗 | — |
| `AllOf/AnyOf/Not` | 組合 | 組合多個 matcher | 多個 matcher | 非致命失敗 | — |

---

## 7) 偵錯與報告

| 介面 | 類別 | 用途 | 輸入參數 | 效果/輸出 | 備註 |
|---|---|---|---|---|---|
| `SCOPED_TRACE(msg)` | 偵錯 | 為區塊內失敗加註上下文 | `msg` 支援串流 | 失敗訊息包含 trace | 疊加可多段 trace |
| `Test::RecordProperty(key, value)` | 報告 | 寫入 XML 屬性 | `key`,`value` | 額外輸出到報告 | test/suite/global 皆可 |

---

## 逐項詳細說明與範例（可用 `g++` 編譯）

> **編譯前置**：系統需有 GoogleTest 函式庫。常見作法：  
> (1) 使用發行版套件；或 (2) 將 googletest 以子模組引入、在專案內建並連結。  
> **最小編譯指令**：`g++ -std=c++23 -O2 your_test.cpp -lgtest -lpthread -o your_test && ./your_test`  
> 若看到 *undefined reference to testing::InitGoogleTest*，請確認連結旗標 `-lgtest -lpthread` 的順序與存在。

### 0) 最小骨架：`main` / 初始化
```cpp
#include <gtest/gtest.h>

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv); // 解析 gtest 旗標
  return RUN_ALL_TESTS();                  // 執行全部測試
}
```

---

### 1) `TEST`：不需要 fixture 的測試
```cpp
#include <gtest/gtest.h>

int Add(int a,int b){ return a+b; }

TEST(MathTest, AddWorks) {
  EXPECT_EQ(Add(2,3), 5);
}
```

---

### 2) `TEST_F` + `SetUp`/`TearDown`：測試治具
```cpp
#include <gtest/gtest.h>
#include <vector>

class VecTest : public ::testing::Test {
protected:
  void SetUp() override { v = {1,2,3}; }
  void TearDown() override { v.clear(); }
  std::vector<int> v;
};

TEST_F(VecTest, Size)     { EXPECT_EQ(v.size(), 3u); }
TEST_F(VecTest, PushBack) { v.push_back(7); ASSERT_EQ(v.back(), 7); }
```

---

### 3) 斷言：`EXPECT_*` vs `ASSERT_*`
```cpp
TEST(AssertVsExpect, Demo) {
  EXPECT_TRUE(2 + 2 == 4); // 失敗不會中止
  ASSERT_NE(1, 2);         // 失敗會中止當前測試
  ASSERT_LT(3, 1);         // 這行失敗 -> 測試函式立刻 return
  SUCCEED();               // 不會執行到
}
```

---

### 4) 字串與浮點
```cpp
TEST(Strings, CStringContent) {
  const char* a = "hello";
  const char* b = "hello";
  EXPECT_STREQ(a, b);              // 比內容
  EXPECT_STRCASEEQ("AbC", "aBc");  // 忽略大小寫
}

TEST(Floats, Near) {
  double a = 0.3, b = 0.1 + 0.2;
  EXPECT_NEAR(a, b, 1e-12);  // 指定容忍
  EXPECT_DOUBLE_EQ(a, b);    // 以 ULP 判等
}
```

---

### 5) 例外：`EXPECT_THROW / NO_THROW / ANY_THROW`
```cpp
#include <stdexcept>

int ParsePositive(int x){
  if (x <= 0) throw std::invalid_argument("x");
  return x;
}

TEST(Exceptions, Basics) {
  EXPECT_THROW   (ParsePositive(0),  std::invalid_argument);
  EXPECT_NO_THROW(ParsePositive(1));
  EXPECT_ANY_THROW(ParsePositive(-9));
}
```

---

### 6) 控制流程：`GTEST_SKIP / SUCCEED / ADD_FAILURE`
```cpp
TEST(Control, SkipOrFail) {
  if (sizeof(void*) == 4) GTEST_SKIP() << "32-bit not supported";
  SUCCEED() << "We are on 64-bit.";
  // 也可在不該到達的分支強制失敗：
  // ADD_FAILURE() << "Unexpected path";
}
```

---

### 7) 參數化：`TEST_P` / `INSTANTIATE_TEST_SUITE_P`
```cpp
#include <gtest/gtest.h>

class IsEvenTest : public ::testing::TestWithParam<int> {};
bool IsEven(int x){ return x % 2 == 0; }

TEST_P(IsEvenTest, Works) {
  EXPECT_EQ(IsEven(GetParam()), (GetParam() % 2 == 0));
}

INSTANTIATE_TEST_SUITE_P(Data, IsEvenTest,
                         ::testing::Values(-2, -1, 0, 1, 2));
```

---

### 8) Matchers：`EXPECT_THAT(actual, matcher)`
```cpp
#include <gtest/gtest.h>
#include <vector>
using ::testing::ElementsAre;
using ::testing::Contains;
using ::testing::HasSubstr;
using ::testing::AllOf;
using ::testing::Not;

TEST(Matchers, ContainersAndStrings) {
  std::vector<int> v{1,2,3};
  EXPECT_THAT(v, ElementsAre(1,2,3));
  EXPECT_THAT(v, Contains(2));

  std::string s = "hello world";
  EXPECT_THAT(s, AllOf(HasSubstr("hello"), HasSubstr("world")));
  EXPECT_THAT(s, Not(HasSubstr("bye")));
}
```

---

### 9) 偵錯與報告：`SCOPED_TRACE` / `RecordProperty`
```cpp
#include <gtest/gtest.h>
#include <vector>

void CheckAllEven(const std::vector<int>& v) {
  for (size_t i = 0; i < v.size(); ++i) {
    SCOPED_TRACE(testing::Message() << "index=" << i); // 失敗時帶上索引
    EXPECT_EQ(v[i] % 2, 0) << "odd value=" << v[i];
  }
}

TEST(Tracing, Scoped) {
  testing::Test::RecordProperty("build_id", "2025-08-19");
  CheckAllEven({2,4,6,7}); // 會在 index=3 失敗並附加 trace
}
```

---

## 編譯與執行（僅 g++）

```bash
# 單檔（含 main 與測試）
g++ -std=c++23 -O2 your_test.cpp -lgtest -lpthread -o your_test
./your_test

# 多檔案：先編物件檔再連結
g++ -std=c++23 -O2 -c test_main.cpp -o test_main.o
g++ -std=c++23 -O2 -c math_test.cpp -o math_test.o
g++ test_main.o math_test.o -lgtest -lpthread -o my_tests
./my_tests
```

> *Acknowledgment:* 本文件部分內容由 ChatGPT 協助產生與整理。
