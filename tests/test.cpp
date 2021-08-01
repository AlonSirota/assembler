#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
#include "./fmemopen_windows.c"
#endif
#include "gtest/gtest.h"
#include "assembler.c"
#include "line.c"
#include "instructionList.c"
#include "directive.c"
#include "symbolTable.c"

TEST(fgetsShred, Shred) {
    FILE *stream;
    char buffer[] = "a\nb";
    char out[40];
    stream = fmemopen(buffer, strlen(buffer), "r");
    fgetsShred(stream, 2+1, out);
    ASSERT_STREQ("a", out) << "Error in reading from stream";
    ASSERT_EQ('b', fgetc(stream)) << "Shred even though reaching \n";
}

TEST(fgetsShred, NoShred) {
    FILE *stream;
    char buffer[] = "ab";
    char out[40];
    stream = fmemopen(buffer, strlen(buffer), "r");
    fgetsShred(stream, 1+1, out);
    ASSERT_STREQ("a", out) << "Error in reading from stream";
    ASSERT_EQ(EOF, fgetc(stream)) << "Didn't shred";
}

TEST(strToLine, easy) {
    char str[] = "label: a b,c";
    line l = strToLine(str);
    ASSERT_STREQ(l.label, "label") << "label error";
    ASSERT_STREQ(l.head.value, "a") << "mnemonic error";
    ASSERT_STREQ(l.head.next->value, "b") << "first parameter error";
    ASSERT_STREQ(l.head.next->next->value, "c") << "post empty parameter error";
    freeLine(l);
}

TEST(strToLine, asciiz_good_syntax) {
    char str[] = "label: .asciz \" word \" ";
    line l = strToLine(str);
    ASSERT_STREQ(l.label, "label") << "label error";
    ASSERT_STREQ(l.head.value, ".asciz") << "mnemonic error";
    ASSERT_STREQ(l.head.next->value, " word ") << "first parameter error";
    freeLine(l);
}

TEST(strToLine, asciiz_extra_chars) {
    char str[] = "label: .asciz \" word \" bad";
    line l = strToLine(str);
    ASSERT_STREQ(l.label, "label") << "label error";
    ASSERT_STREQ(l.head.value, ".asciz") << "mnemonic error";
    ASSERT_STREQ(l.head.next->value, " word ") << "first parameter error";
    ASSERT_EQ(NULL, l.head.next->next) << "Post string input error";
    ASSERT_EQ(l.error, ASCIIZ_EXTRA_TOKENS) << "Extra tokens not detected";
    freeLine(l);
}

TEST(strToLine, asciiz_unbalanced_parenthesis) {
    char str[] = "label: .asciz \" word ";
    line l = strToLine(str);
    ASSERT_STREQ(l.label, "label") << "label error";
    ASSERT_STREQ(l.head.value, ".asciz") << "mnemonic error";
    ASSERT_EQ(NULL, l.head.next) << "first parameter error";
    ASSERT_EQ(l.error,ASCIIZ_UNBALANCED_PARENTHESIS) << "Unbalanced parenthesis error not flagged";
    freeLine(l);
}

TEST(strToLine, asciiz_missing_parenthesis) {
    char str[] = "label: .asciz word";
    line l = strToLine(str);
    ASSERT_STREQ(l.label, "label") << "label error";
    ASSERT_STREQ(l.head.value, ".asciz") << "mnemonic error";
    ASSERT_EQ(NULL, l.head.next) << "first parameter error";
    ASSERT_EQ(l.error,ASCIIZ_MISSING_PARENTHESIS) << "Unbalanced parenthesis error not flagged";
    freeLine(l);
}

TEST(strToLine, emptyLabel) {
    char str[] = ": a b,c";
    line l = strToLine(str);
    ASSERT_STREQ(l.label, "") << "label error";
    ASSERT_STREQ(l.head.value, "a") << "mnemonic error";
    ASSERT_STREQ(l.head.next->value, "b") << "first parameter error";
    ASSERT_STREQ(l.head.next->next->value, "c") << "post empty parameter error";
    freeLine(l);
}

TEST(strToLine, noLabel) {
    char str[] = "a b,c";
    line l = strToLine(str);
    ASSERT_STREQ(l.head.value, "a") << "mnemonic error";
    ASSERT_STREQ(l.head.next->value, "b") << "first parameter error";
    ASSERT_STREQ(l.head.next->next->value, "c") << "post empty parameter error";
    freeLine(l);
}

TEST(strToLine, emptyParameters) {
    char str[] = "label: a b,, c";
    line l = strToLine(str);
    ASSERT_STREQ(l.label, "label") << "label error";
    ASSERT_STREQ(l.head.value, "a") << "mnemonic error";
    ASSERT_STREQ(l.head.next->value, "b") << "first parameter error";
    ASSERT_STREQ(l.head.next->next->value, "") << "empty parameter error";
    ASSERT_STREQ(l.head.next->next->next->value, "c") << "post empty parameter error";
    freeLine(l);
}

TEST(strToLine, emptyLine) {
    char str[] = "";
    line l = strToLine(str);
    ASSERT_TRUE(l.label == NULL) << "label error";
    ASSERT_TRUE(l.head.value == NULL) << "mnemonic error";
    ASSERT_TRUE(l.head.next == NULL) << "first parameter error";
    freeLine(l);
}

TEST(strToLine, whiteSpaceLine) {
    char str[] = " \t";
    line l = strToLine(str);
    ASSERT_TRUE(l.label == NULL) << "label error";
    ASSERT_TRUE(l.head.value == NULL) << "mnemonic error";
    ASSERT_TRUE(l.head.next == NULL) << "first parameter error";
    freeLine(l);
}

TEST(strToLine, commentLine) {
    char str[] = "; label: a b,c";
    line l = strToLine(str);
    ASSERT_TRUE(l.label == NULL) << "label error";
    ASSERT_TRUE(l.head.value == NULL) << "mnemonic error";
    ASSERT_TRUE(l.head.next == NULL) << "first parameter error";
    freeLine(l);
}

TEST(trimWhiteSpace, leading) {
    char str[] = " \t\na \t\n";
    ASSERT_STREQ(firstNoneSpace(str), "a \t\n");
}

TEST(trimWhiteSpace, trailing) {
    char str[] = " \t\na \t\n";
    trimTrailingSpace(str);
    ASSERT_STREQ(str, " \t\na");
}

TEST(trimWhiteSpace, both) {
    char str[] = " \t\na \t\n";
    char *trimmed = trimWhiteSpace(str);
    ASSERT_STREQ(trimmed, "a");
}
TEST(findIntruction, noLable){
    inst *instruction = findInstruction("add");
    ASSERT_EQ(instruction == NULL, 0);
    instruction = findInstruction("lol");
    ASSERT_NE(instruction == NULL, 0);
}
TEST(parseRInstructions, noLabel){
    char buf[20];
    char *output = "40 48 65 00"; /* see page 27 first intruction in table*/
    char codeSeg[4];
    int status, ic = 100;
    char str[] = "add $3,$5,$9";
    line l = strToLine(str);
    inst *instruction = findInstruction(l.head.value);
    status = parseRInstruction(instruction, l.head.next, buf);
    ASSERT_EQ(status, LINE_OK);
    ASSERT_EQ(strcmp(output,buf), 0);
}

TEST(addSymbol, easy) {
    Symbol *tableHead = NULL; /* aka empty */
    addSymbol(&tableHead, "a", 1, DATA);
    addSymbol(&tableHead, "b", 2, DATA | CODE);

    Symbol *curr = tableHead;
    ASSERT_TRUE(curr != NULL);
    ASSERT_TRUE(!strcmp(curr->label, "a"));
    ASSERT_TRUE(curr->address == 1);
    ASSERT_TRUE(curr->attributes == DATA);
    curr = curr->next;
    ASSERT_TRUE(curr != NULL);
    ASSERT_TRUE(!strcmp(curr->label, "b"));
    ASSERT_TRUE(curr->address == 2);
    ASSERT_TRUE(curr->attributes == DATA | CODE);
    ASSERT_TRUE(curr->next == NULL);
    discardTable(tableHead);
}

TEST(addSymbol, preventDuplicates) {
    Symbol *tableHead = NULL; /* aka empty */
    addSymbol(&tableHead, "a", 1, DATA);
    addSymbol(&tableHead, "a", 2, DATA | CODE);

    Symbol *curr = tableHead;
    ASSERT_TRUE(curr != NULL);
    ASSERT_TRUE(!strcmp(curr->label, "a"));
    ASSERT_TRUE(curr->address == 1);
    ASSERT_TRUE(curr->attributes == DATA);
    ASSERT_TRUE(curr->next == NULL);
    discardTable(tableHead);
}

TEST(directiveToByteTest, db) {
    byte buffer[1000];
    byte *res;
    node nextNext = {.value = "-12", .next = NULL};
    node next = {.value = "31", .next = &nextNext};
    node head = {.value = ".db", .next = &next};
    line l = {.head = head};
    res = directiveToBytes(l);
    ASSERT_TRUE(res[0] == 0b00011111); /* 32 in binary */
    ASSERT_TRUE(res[1] == 0b11110100); /* -12 in binary */

    free(res);
}
TEST(directiveToByteTest, dw) {
    byte buffer[1000];
    byte *res;
    node nextNext = {.value = "-12", .next = NULL};
    node next = {.value = "31", .next = &nextNext};
    node head = {.value = ".dw", .next = &next};
    line l = {.head = head};
    res = directiveToBytes(l);

    ASSERT_TRUE(res[0] == 0b00011111); /* 32 in binary */
    ASSERT_TRUE(res[1] == 0);
    ASSERT_TRUE(res[2] == 0);
    ASSERT_TRUE(res[3] == 0);

    ASSERT_TRUE(res[4] == 0b11110100); /* -12 in binary */
    ASSERT_TRUE(res[5] == 0b11111111);
    ASSERT_TRUE(res[6] == 0b11111111);
    ASSERT_TRUE(res[7] == 0b11111111);

    free(res);
}
TEST(directiveToByteTest, dh) {
    byte buffer[1000];
    byte *res;
    node nextNext = {.value = "-12", .next = NULL};
    node next = {.value = "31", .next = &nextNext};
    node head = {.value = ".dh", .next = &next};
    line l = {.head = head};
    res = directiveToBytes(l);

    ASSERT_TRUE(res[0] == 0b00011111); /* 32 in binary */
    ASSERT_TRUE(res[1] == 0b00000000);

    ASSERT_TRUE(res[2] == 0b11110100); /* -12 in binary */
    ASSERT_TRUE(res[3] == 0b11111111);
    free(res);
}
