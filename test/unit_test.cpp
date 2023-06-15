/*
MIT License

Copyright (c) 2023 https://github.com/yuriiq

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "signal_handler.h"

class Test1 : BEGIN_OBJECT(Test1)
public:
  virtual ~Test1() = default;
  virtual void test_slot(double) {};
  SIGNAL(test_signal, double);
  void test_imit(double data) {
    EMIT(&Test1::test_signal,  data);
  }
END_OBJECT()

class MockTest1 : public Test1 
{
 public:
   MOCK_METHOD(void, test_slot, (double d), (override));
};

///
class Test2: BEGIN_OBJECT(Test2) 
public:
  virtual ~Test2() = default;
  virtual void test_slot(double) {};
  SIGNAL(test_signal, double);
  void test_imit(double data) {
    EMIT(&Test2::test_signal,  data);
  }
END_OBJECT()

class MockTest2 : public Test2
{
 public:
   MOCK_METHOD(void, test_slot, (double d), (override));
};

///
class Test3: BEGIN_OBJECT(Test3) 
public:

  SIGNAL(test_signal, double);
  void test_imit(double data) {
    EMIT(&Test3::test_signal,  data);
  }
END_OBJECT()

namespace {

TEST(SignalHandler, connect_disconnect) {
  MockTest1 test1;
  Test2 test2;
  double data = 3.14;
  EXPECT_CALL(test1, test_slot(data)).Times(1);

  Test1::connect(test2, &Test2::test_signal, test1, &Test1::test_slot);
  test2.test_imit(data);
  Test1::disconnect(test2, &Test2::test_signal, test1, &Test1::test_slot);
  test2.test_imit(data);
}

TEST(SignalHandler, self_connect) {
  MockTest1 test1;
  MockTest1 test2;
  double data = 3.14;
  EXPECT_CALL(test1, test_slot(data)).Times(1);

  Test1::connect(test2, &MockTest1::test_signal, test1, &MockTest1::test_slot);
  test2.test_imit(data);
  Test1::disconnect(test2, &MockTest1::test_signal, test1, &MockTest1::test_slot);
  test2.test_imit(data);
}

TEST(SignalHandler, multislot_connect) {
  MockTest1 test1;
  MockTest2 test2;
  Test3 test3;
  double data = 3.14;
  EXPECT_CALL(test1, test_slot(data)).Times(1);
  EXPECT_CALL(test2, test_slot(data)).Times(1);

  Test1::connect(test3, &Test3::test_signal, test1, &MockTest1::test_slot);
  Test1::connect(test3, &Test3::test_signal, test2, &MockTest2::test_slot);
  test3.test_imit(data);
}

TEST(SignalHandler, multisignal_connect) {
  MockTest1 test1;
  Test2 test2;
  Test3 test3;
  double data = 3.14;
  EXPECT_CALL(test1, test_slot(data)).Times(2);
  Test1::connect(test2, &Test2::test_signal, test1, &MockTest1::test_slot);
  Test1::connect(test3, &Test3::test_signal, test1, &MockTest1::test_slot);
  test3.test_imit(data);
  test2.test_imit(data);
}

TEST(SignalHandler, auto_disconnect) {
  MockTest1 * test1 = new MockTest1;
  Test2 test2;
  double data = 3.14;
  EXPECT_CALL(*test1, test_slot(data)).Times(1);
  Test1::connect(test2, &Test2::test_signal, *test1, &MockTest1::test_slot);
  test2.test_imit(data);
  delete test1;
  test2.test_imit(data);
}

}
