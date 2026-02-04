#include <gtest/gtest.h>

#include "lexer.h"

TEST(LexerTest, EmptySource) {
  Lexer lexer("");
  EXPECT_FALSE(lexer.hasMoreTokens());
}

TEST(LexerTest, Placeholder) {
  // Placeholder for future lexer tests
  EXPECT_TRUE(true);
}
