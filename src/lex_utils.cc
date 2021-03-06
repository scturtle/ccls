#include "lex_utils.h"

#include <doctest/doctest.h>

#include <algorithm>

// VSCode (UTF-16) disagrees with Emacs lsp-mode (UTF-8) on how to represent
// text documents.
// We use a UTF-8 iterator to approximate UTF-16 in the specification (weird).
// This is good enough and fails only for UTF-16 surrogate pairs.
int GetOffsetForPosition(lsPosition position, std::string_view content) {
  size_t i = 0;
  for (; position.line > 0 && i < content.size(); i++)
    if (content[i] == '\n')
      position.line--;
  for (; position.character > 0 && i < content.size(); position.character--)
    if (uint8_t(content[i++]) >= 128) {
      // Skip 0b10xxxxxx
      while (i < content.size() && uint8_t(content[i]) >= 128 &&
             uint8_t(content[i]) < 192)
        i++;
    }
  return int(i);
}

std::string_view LexIdentifierAroundPos(lsPosition position,
                                        std::string_view content) {
  int start = GetOffsetForPosition(position, content);
  int end = start + 1;
  char c;

  // We search for :: before the cursor but not after to get the qualifier.
  for (; start > 0; start--) {
    c = content[start - 1];
    if (isalnum(c) || c == '_')
      ;
    else if (c == ':' && start > 1 && content[start - 2] == ':')
      start--;
    else
      break;
  }

  for (; end < (int)content.size(); end++)
    if (c = content[end], !(isalnum(c) || c == '_'))
      break;

  return content.substr(start, end - start);
}

// Find discontinous |search| in |content|.
// Return |found| and the count of skipped chars before found.
int ReverseSubseqMatch(std::string_view pat,
                       std::string_view text,
                       int case_sensitivity) {
  if (case_sensitivity == 1)
    case_sensitivity = std::any_of(pat.begin(), pat.end(), isupper) ? 2 : 0;
  int j = pat.size();
  if (!j)
    return text.size();
  for (int i = text.size(); i--;)
    if ((case_sensitivity ? text[i] == pat[j - 1]
                          : tolower(text[i]) == tolower(pat[j - 1])) &&
        !--j)
      return i;
  return -1;
}

TEST_SUITE("Offset") {
  TEST_CASE("past end") {
    std::string content = "foo";
    int offset = GetOffsetForPosition(lsPosition{10, 10}, content);
    REQUIRE(offset <= content.size());
  }

  TEST_CASE("in middle of content") {
    std::string content = "abcdefghijk";
    for (int i = 0; i < content.size(); ++i) {
      int offset = GetOffsetForPosition(lsPosition{0, i}, content);
      REQUIRE(i == offset);
    }
  }

  TEST_CASE("at end of content") {
    REQUIRE(GetOffsetForPosition(lsPosition{0, 0}, "") == 0);
    REQUIRE(GetOffsetForPosition(lsPosition{0, 1}, "a") == 1);
  }
}
