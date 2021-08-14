//
// Created by alon on 6/27/21.
//
#include "assembler.h"

#define CODE_SIZE 2^25 /* max memory size */

#define DATA_SIZE 2^25

char parseOp(node node, char string[18], int i, Symbol *pSymbol);

void logError(enum ErrorCode error, int *hasErrors, int lineNumber);

const char *codeToMsg(enum ErrorCode code);

void assemblePath(char *fileName) {
    // TODO enforce '.as' file extension.
    FILE *f = fopen(fileName, "r");
    if (f == NULL) {
        printf("Error while opening file %s\n", fileName);
    }
    assembleFile(f);
    fclose(f);
}

/*
 * Assembles file f.
 * f parameter is expected to be an opened file.
 * f won't be closed as part of this function.
 */
void assembleFile(FILE *f) {
    assert(f != NULL);
    firstPass(f);
    // TODO Second pass
}

void firstPass(FILE *f) {
    int ic = 100, dc = 0, lineNumber;
    char lineStr[LINE_LENGTH + 1];
    line lineParsed;
    Symbol *symbolTable = NULL;
    bytesNode *dataImage = NULL;
    byte *directiveBytes = NULL;
    int hasErrors = FALSE;
    enum ErrorCode error = GOOD; /* If encountered error */

    assert(f != NULL);

    /* Iterate over file's lines, while counting the line number */
    for (lineNumber = 0; fgetsShred(f, LINE_LENGTH + 1, lineStr); lineNumber++) {
        lineParsed = strToLine(lineStr);

        if (!isLineRelevant(lineParsed)) {
            continue;
        }
        else if (isLineDirective(lineParsed)) {
            if (lineParsed.label != NULL) {
                error = addSymbol(&symbolTable, lineParsed.label, dc, DATA);
                logError(error, &hasErrors, lineNumber);
            }

            /* Encode directive data */
            directiveBytes = directiveToBytes(lineParsed, &error);
            if (directiveBytes)
            {
                addBytesToImage(&dataImage, directiveBytes);
                dc += sizeof(directiveBytes);
            }
            /* continue */
        }
        else if (!strcmp(lineParsed.label,ENTRY_MNEMONIC)) {
            continue; /* Not handled in first pass. */
        } else if (!strcmp(lineParsed.label,EXTERN_MNEMONIC)) {
            error = processExtern(lineParsed.head, &symbolTable, dc);
            logError(error, &hasErrors, lineNumber);
        } else { /* Treat this line as an instruction, all other options have been eliminated. */
            if (lineParsed.label != NULL) {
                error = addSymbol(&symbolTable, lineParsed.label, ic, CODE);
                logError(error, &hasErrors, lineNumber);
                ic += 4;
            }
            /* Further processing of instruction line is done in second pass. */
        }
    }
}

/*
 * Prints an error message like this:
 * also sets hasErrors to true if errorCode is an actual error.
 * "3: message\n"
 */
void logError(enum ErrorCode error, int *hasErrors, int lineNumber) {
    const char* errorMsg;
    if (error != GOOD) {
        errorMsg = codeToMsg(error);
        printf("%d: %s\n", lineNumber, errorMsg);

        if (error) {
            *hasErrors = TRUE;
        }
    }
}

const char *codeToMsg(enum ErrorCode code) {
    /* TODO, switch case returning a string describing this error code */
    return "temp message";
}

/*
 * Like fgets, but flushes f till end of line / EOF.
 */
char *fgetsShred(FILE *f, int n, char *buffer) {
    int i, ch, reachedEndOfLine = FALSE;

    ch = getc(f);
    if (n <= 0 || ch == EOF) { /* If nothing to be read, or EOF already reached before reading */
        return NULL;
    }
    ungetc(ch, f); /* Pushback to be read in loop later */
    n--; /* leave space fo NULL */

    for (i = 0; i < n; i++) {
        ch = getc(f);

        if (ch == EOF || ch == '\n') {
            reachedEndOfLine = TRUE;
            break;
        } else {
            buffer[i] = ch;
        }
    }
    buffer[i] = '\0';

    while (!reachedEndOfLine) { /* until reaching end of line */
        /* Shred: Read trailing characters, to avoid reading them later as a new line */
        ch = getc(f);
        if (ch == EOF || ch == '\n') {
            reachedEndOfLine = TRUE;
        }
    }

    return buffer;
}

void secondPass(FILE *f){
    Symbol *symbolTable; /* will be recived from first pass */
    int ic = 100;
    char codeSeg[CODE_SIZE];
    char dataSeg[DATA_SIZE]; /* will be recived from first pass */
    char lineStr[LINE_LENGTH + 1];
    line lineParsed;
    assert(f != NULL);
    fgetsShred(f, LINE_LENGTH + 1, lineStr);
    lineParsed = strToLine(lineStr);
    parseOp(lineParsed.head, codeSeg, ic, symbolTable);
    /*if (!instruction){
        codeSeg[ic-100] = instruction
    }*/
}

char parseOp(node node, char string[18], int i, Symbol *pSymbol) {

    return 0;
}

void generateOutput(FILE *f, char *codeSeg, int ic, int dc, char *dataSeg){
    int i, num = 100;
    fprintf(f, "%d %d\n", ic-100,dc);
    while (num <= ic){
        fprintf(f, "%.4d", num);
        for(i = 0; i < 4 && num++ <= ic; i++) {
            fprintf(f, " %.2X", codeSeg[num - 100]);
        }
        printf("/n");
    }
}

int addBytesToImage(bytesNode **tablePtr, byte *bytes) {
    bytesNode * curr;
    bytesNode *next = (bytesNode *) malloc(sizeof (bytesNode)); /* Prepare new symbol */
    // TODO makesure malloc succeded
    next->next = NULL;
    next->bytes = bytes;

    if (*tablePtr == NULL) { /* if table is empty */
        *tablePtr = next; /* it now contains the new element. */
        return EXIT_SUCCESS;
    }

    /* else, find last element*/
    curr = *tablePtr;
    while (curr->next != NULL)
        curr = curr->next;

    /* append */
    curr->next = next;
    return EXIT_SUCCESS;
}

enum ErrorCode processExtern(node operandHead, Symbol **symbolTablePtr, int dc) {
    if (operandHead.value == NULL) {
        return MISSING_OPERAND;
    } else if (isValidLabel(operandHead.value)) {
        return addSymbol(symbolTablePtr, operandHead.value, dc, EXTERNAL);
    } else {
        return INVALID_LABEL;
    }
}

/*
 * Returns true if this line is relevant (and should be processed).
 * Otherwise it's a comment or empty.
 */
int isLineRelevant(line l) {
    /* if mnemonic is empty */
    return !(l.head.value == NULL || !strcmp(l.head.value, ""));
}
