# Google Test 常用函式／巨集速查（Quick Reference）

> 下面先列**常用介面與說明**；詳細解說與可編譯範例請往下看。

### 定義測試 / 測試治具
- `TEST(Suite, Name)`：定義不需 fixture 的測試。
- `TEST_F(FixtureClass, Name)`：使用測試治具（`SetUp`/`TearDown`）。
- `TEST_P(FixtureWithParam, Name)` + `INSTANTIATE_TEST_SUITE_P(...)`：參數化測試。
- `::testing::InitGoogleTest(&argc, argv);` / `RUN_ALL_TESTS()`：初始化並執行全部測試。

### 一般斷言（EXPECT_* 非致命；ASSERT_* 致命、會立即中止該測試）
- 布林：`EXPECT_TRUE(x)` / `EXPECT_FALSE(x)`、`ASSERT_TRUE(x)` / `ASSERT_FALSE(x)`
- 相等／不等：`EXPECT_EQ(a,b)` / `EXPECT_NE(a,b)`、`ASSERT_EQ` / `ASSERT_NE`
- 比較：`EXPECT_LT/LE/GT/GE(a,b)`、對應 `ASSERT_LT/LE/GT/GE`

### 字串與浮點
- C 字串內容：`EXPECT_STREQ/STRNE(s1,s2)`、忽略大小寫 `EXPECT_STRCASEEQ/STRCASENE`
- 浮點：`EXPECT_FLOAT_EQ` / `EXPECT_DOUBLE_EQ`、或 `EXPECT_NEAR(a,b,abs_error)`

### 例外
- `EXPECT_THROW(stmt, Ex)`、`EXPECT_NO_THROW(stmt)`、`EXPECT_ANY_THROW(stmt)`
- `ASSERT_THROW/NO_THROW/ANY_THROW`

### 控制流程
- `GTEST_SKIP()`：在執行期跳過目前測試。
- `SUCCEED()`：顯式標記成功（不常用）。
- `ADD_FAILURE()`：強制標記失敗（非致命）。

### Matchers（搭配 `EXPECT_THAT(actual, matcher)`）
- 常用：`ElementsAre(...)`、`Contains(x)`、`UnorderedElementsAre(...)`、`HasSubstr("...")`、`StartsWith/EndsWith`
- 組合：`AllOf(...)`、`AnyOf(...)`、`Not(m)`

### 偵錯與報告
- `SCOPED_TRACE(msg)`：加註堆疊範圍內失敗的額外訊息。
- `RecordProperty(key, value)`：輸出到 XML 報告屬性（在 test/test suite/global）。

### 進階
- 全域環境：`testing::AddGlobalTestEnvironment(new Env);`
- 事件監聽器：自訂 `TestEventListener` / `EmptyTestEventListener` 加入 `UnitTest::GetInstance()->listeners()`

---

## 詳細說明與範例

> 以下範例可在 **Ubuntu / WSL** 下以 `g++ -std=c++23 -lgtest -lpthread` 編譯執行，或用 CMake。

### 0) 最小骨架：main / 初始化
```cpp
#include <gtest/gtest.h>

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv); // 解析 gtest 旗標
  return RUN_ALL_TESTS();                  // 執行全部測試
}
```

---

### 1) 定義測試：`TEST`
```cpp
#include <gtest/gtest.h>

int Add(int a,int b){ return a+b; }

TEST(MathTest, AddWorks) {
  EXPECT_EQ(Add(2,3), 5);
}
```

---

### 2) 測試治具（Fixture）：`TEST_F` + `SetUp`/`TearDown`
```cpp
#include <gtest/gtest.h>
#include <vector>

class VecTest : public ::testing::Test {
protected:
  void SetUp() override { v = {1,2,3}; }
  void TearDown() override { v.clear(); }
  std::vector<int> v;
};

TEST_F(VecTest, Size) { EXPECT_EQ(v.size(), 3u); }
TEST_F(VecTest, PushBack) { v.push_back(7); ASSERT_EQ(v.back(), 7); }
```

---

### 3) 斷言（EXPECT_* vs ASSERT_*）
- `EXPECT_*`：失敗**不中止**當前測試函式。
- `ASSERT_*`：失敗**立即 return** 當前測試函式（致命）。

```cpp
TEST(AssertVsExpect, Demo) {
  EXPECT_TRUE(2 + 2 == 4); // 繼續
  ASSERT_NE(1, 2);         // 繼續
  ASSERT_LT(3, 1);         // 這行失敗 -> 測試函式立刻返回
  SUCCEED();               // 不會被執行
}
```

---

### 4) 字串與浮點
**C 字串內容比較**（不要用 `EXPECT_EQ` 比 `const char*` 指標）：
```cpp
TEST(Strings, CStringContent) {
  const char* a = "hello";
  const char* b = "hello";
  EXPECT_STREQ(a, b);            // 內容相等
  EXPECT_STRCASEEQ("AbC", "aBc"); // 忽略大小寫
}
```

**浮點比較**：
```cpp
TEST(Floats, Near) {
  double a = 0.3, b = 0.1 + 0.2;
  EXPECT_NEAR(a, b, 1e-12);
  EXPECT_DOUBLE_EQ(a, b); // 使用 ULP 規則
}
```

---

### 5) 例外
```cpp
#include <stdexcept>

int ParsePositive(int x){
  if (x <= 0) throw std::invalid_argument("x");
  return x;
}

TEST(Exceptions, Basics) {
  EXPECT_THROW(ParsePositive(0), std::invalid_argument);
  EXPECT_NO_THROW(ParsePositive(1));
  EXPECT_ANY_THROW(ParsePositive(-9));
}
```

---

### 6) 控制流程：跳過／成功／強制失敗
```cpp
TEST(Control, SkipOrFail) {
  if (sizeof(void*) == 4) GTEST_SKIP() << "32-bit not supported";
  SUCCEED() << "We are on 64-bit.";
  // 你也可在不該到達的分支強制失敗：
  // ADD_FAILURE() << "Unexpected path";
}
```

---

### 7) 參數化測試：`TEST_P` / `INSTANTIATE_TEST_SUITE_P`
```cpp
#include <gtest/gtest.h>

class IsEvenTest : public ::testing::TestWithParam<int> {};
bool IsEven(int x){ return x%2==0; }

TEST_P(IsEvenTest, Works) {
  EXPECT_EQ(IsEven(GetParam()), (GetParam()%2==0));
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

TEST(Matchers, ContainersAndStrings) {
  std::vector<int> v{1,2,3};
  EXPECT_THAT(v, ElementsAre(1,2,3));
  EXPECT_THAT(v, Contains(2));

  std::string s = "hello world";
  EXPECT_THAT(s, AllOf(HasSubstr("hello"), HasSubstr("world")));
}
```

---

### 9) 偵錯與報告：`SCOPED_TRACE` / `RecordProperty`
```cpp
#include <gtest/gtest.h>
#include <vector>

void CheckAllEven(const std::vector<int>& v) {
  for (size_t i = 0; i < v.size(); ++i) {
    SCOPED_TRACE(testing::Message() << "index=" << i);
    EXPECT_EQ(v[i] % 2, 0) << "odd value=" << v[i];
  }
}

TEST(Tracing, Scoped) {
  testing::Test::RecordProperty("build_id", "2025-08-19");
  CheckAllEven({2,4,6,7}); // 失敗訊息會包含 index 與自訂訊息
}
```

---

### 10) 全域環境：`AddGlobalTestEnvironment`
```cpp
#include <gtest/gtest.h>

struct MyEnv : testing::Environment {
  void SetUp() override    { /* global init once */ }
  void TearDown() override { /* global cleanup once */ }
};

int main(int argc, char** argv){
  ::testing::InitGoogleTest(&argc, argv);
  testing::AddGlobalTestEnvironment(new MyEnv());
  return RUN_ALL_TESTS();
}
```

---

### 11) 事件監聽器：`TestEventListener`
```cpp
#include <gtest/gtest.h>

class MyListener : public ::testing::EmptyTestEventListener {
  void OnTestStart(const ::testing::TestInfo& ti) override {
    fprintf(stderr, "[RUN ] %s.%s\n", ti.test_suite_name(), ti.name());
  }
  void OnTestEnd(const ::testing::TestInfo& ti) override {
    fprintf(stderr, "[DONE] %s.%s\n", ti.test_suite_name(), ti.name());
  }
};

int main(int argc,char** argv){
  ::testing::InitGoogleTest(&argc, argv);
  auto& ls = ::testing::UnitTest::GetInstance()->listeners();
  ls.Append(new MyListener()); // gtest 會在程式結束時釋放
  return RUN_ALL_TESTS();
}
```

---

### 12) 執行旗標（命令列）
- 僅執行特定測試：`--gtest_filter=SuiteName.TestName`
- 萬用字元：`--gtest_filter=Math*.*-*.Slow*`（先包含、後以 `-` 排除）
- 亂序執行：`--gtest_shuffle`
- 重複執行 N 次：`--gtest_repeat=10`
- 第一次失敗就停止：`--gtest_break_on_failure`
- 產出 XML 報告：`--gtest_output=xml:report.xml`

例：
```bash
./my_tests --gtest_filter=VecTest.* --gtest_shuffle --gtest_repeat=5
```

---

### 13) CMake / 編譯（WSL/Ubuntu）
**CMake 最小例：**
```cmake
cmake_minimum_required(VERSION 3.20)
project(gtest_demo CXX)
set(CMAKE_CXX_STANDARD 23)

find_package(GTest REQUIRED)
add_executable(my_tests test_main.cpp math_test.cpp)
target_link_libraries(my_tests PRIVATE GTest::gtest GTest::gtest_main pthread)
```

**g++ 直接編：**
```bash
g++ -std=c++23 -O2 my_tests.cpp -lgtest -lpthread -o my_tests
./my_tests
```

---


> *Acknowledgment:* 本文件部分內容由 ChatGPT 協助產生與整理。

> *Acknowledgment:* 本文件部分內容由 ChatGPT 協助產生與整理。
