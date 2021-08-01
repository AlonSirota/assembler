#include "line.h"
#include <stddef.h>
#include <string.h>
#include <stdio.h>

char* strdupN(char *original, int n) {
    char *out = (char *) malloc(sizeof(char) * (n + 1)); /* + 1 for '\0' */
    int i;
    for (i = 0; i < n; i++) {
        out[i] = original[i];
    }
    out[i] = '\0';
    return out;
}

char* strsep(char** stringp, const char* delim)
{
  char* start = *stringp;
  char* p;

  p = (start != NULL) ? strpbrk(start, delim) : NULL;

  if (p == NULL)
  {
    *stringp = NULL;
  }
  else
  {
    *p = '\0';
    *stringp = p + 1;
  }

  return start;
}

/*
 * Parse a string to a line struct
 * Returned struct contains malloced pointers and must be released.
 * ex. line = strToLine("label: a b,c ;comment")
 * line.label = "label"
 * line.head = a->b->c->NULL
 */
line strToLine(char *str) {
    line l = {.label = NULL, .head = { .value = NULL, .next = NULL}, l.error = GOOD};
    char *token, *savePtr;
    node *curr;

    if (str[0] == ';') {/* Comment line, return empty line */
        return l;
    }

    token = strtok_r(str, WORD_DELIMITERS, &savePtr);
    if (token == NULL) { /* Line without words, return empty line */
        return l;
    }

    if (lastChar(token) == LABEL_SUFFIX) { /* If first word is a label */
        l.label = strdup(token);
        l.label[strlen(l.label) - 1] = '\0'; /* Trim the ':' from the label name */

        /* Next word */
        token = strtok_r(NULL, WORD_DELIMITERS, &savePtr);
        if (token == NULL) {
            return l;
        }
    }

    /* Token is now mnemonic (either the first word, or the first word after the label  */
    l.head.value = strdup(token);

    parseParameters(savePtr, &l);

    return l;
}

/*
 * Returns
 * Expects str to have at least one character.
 */
char lastChar(char *str) {
    assert(str != NULL);
    assert(strlen(str));

    return str[strlen(str) - 1];
}

/*
 * Returns pointer first none whitespace char in str.
 */
char * firstNoneSpace(char *str) {
    assert(str != NULL);
    while (*str != '\0' && isspace(*str)) {
        str++;
    }
    return str;
}

/*
 * Inserts '\0' after the last none-whitespace character.
 * Modifies parameter str.
 */
void trimTrailingSpace(char *str) {
    int i, lastGraph = NONE;
    assert(str != NULL);

    /* Find last graphical character index */
    for (i = 0; str[i] != '\0'; i++) {
        if (!isspace(str[i])) {
            lastGraph = i;
        }
    }

    if (lastGraph != NONE){ /* If there was a graphical character */
        str[lastGraph + 1] = '\0'; /* It is now the last character */
    }    
}

/*
 * Returns pointer to str without trailing and leading whitespace.
 * Points to a character in the parameter (not creating a semi-duplicate).
 */
char *trimWhiteSpace(char *str) {
    trimTrailingSpace(str);
    return firstNoneSpace(str);
}

void freeLine(line l) {
    node *curr, *next;

    freeSafely(l.label);
    freeSafely(l.head.value);

    curr = l.head.next;
    for (curr = l.head.next; curr != NULL; curr = next) {
        freeSafely(curr->value);
        next = curr->next;
        freeSafely(curr);
    }
}

void freeSafely(void *ptr) {
    if (ptr != NULL)
        free(ptr);
}

/*
 * Parse paramStr as the parameters in an .asciiz directive,
 * populate lOut's parameters accordingly.
 */
void parseAscizParameters(char *paramStr, line *lOut) {
    char *token;
    node *curr;
    token = strtok_r(NULL,"", &paramStr);
    if (*token != '\"') {
        printf("Expected \" as first character in first token after .asciz\n");
        lOut->error = ASCIIZ_MISSING_PARENTHESIS;
    } else {
        token++; /* skip first \" char */
        char *closingParenthesisPtr = strpbrk(token, "\"");
        if (closingParenthesisPtr == NULL) {
            lOut->error = ASCIIZ_UNBALANCED_PARENTHESIS;
        } else { /* Found closing \" character */
            curr = (node *) malloc(sizeof (node));
            curr->next = NULL;
            curr->value = strdupN(token, closingParenthesisPtr - token); /* -1 because we it mustn't copy the '\"' char. */
            lOut->head.next = curr;

            /* Make sure there aren't more tokens after closing parenthesis */
            trimWhiteSpace(closingParenthesisPtr);
            if (*(closingParenthesisPtr+1) != '\0') {
                lOut->error = ASCIIZ_EXTRA_TOKENS;
            }
        }
    }
}

/*
 * Parse paramStr as the parameters in the format of '[whitespace] word [whitespace],[whitespace] word...'
 * populate lOut's parameters accordingly.
 */
void parseGenericParameters(char *paramStr, line *lOut) {
    node *curr;
    char *token;
    curr = &lOut->head;
    do {
        token = strsep(&paramStr, PARAMETER_DELIM);
        if (token) {
            token = trimWhiteSpace(token);
            curr->next = (node *) malloc(sizeof (node));
            curr = curr->next;
            curr->value = strdup(token);
        }
    } while (token);
    curr->next = NULL;
}

/*
 * Parse paramStr as the parameters in the format that matches the mnemonic in lOut.
 * populate lOut's parameters accordingly.
 */
void parseParameters(char *paramStr, line *lOut) {
    if(!strcmp(ASCII_MNEMONIC, lOut->head.value)) { /* ASCII directive requires unique line parsing */
        parseAscizParameters(paramStr, lOut);
    } else {
        parseGenericParameters(paramStr, lOut);
    }
}