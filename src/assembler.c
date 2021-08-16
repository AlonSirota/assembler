//
// Created by alon on 6/27/21.
//
#include "assembler.h"
#include "instructionList.h"



void assemblePath(char *fileName) {
    /* Check that fileName ends with .asm */
    char *extension = strstr(fileName, ASSEMBLY_EXTENSION);
    if (extension == NULL) {
        printf("%s is not a valid assembly file name\n", fileName);
        return;
    }

    FILE *f = fopen(fileName, "r");
    if (f == NULL) {
        printf("Error while opening file %s\n", fileName);
        return;
    }

    *extension = '\0'; /* Trim .asm extension */
    assembleFile(f, fileName);
    fclose(f);
}


/*
 * Assembles file f.
 * f parameter is expected to be an opened file.
 * f won't be closed as part of this function.
 */
void assembleFile(FILE *f, char *fileName) {
    assert(f != NULL);
    enum ErrorCode e1, e2, e3, e4;
    bytesNode *dataImage;
    int ic, dc;
    FILE *objFile;
    char objFileName[MAX_FILE_NAME_LEN];
    Symbol *symbolTable;
    externalTable  *externalTable1;

    if (firstPass(f, &ic, &dc, &dataImage, &symbolTable) == GOOD) {
        /* Name of object file is: "<.as file name>.ob" */
        strcpy(objFileName, fileName);
        strcat(objFileName, ".ob");

        /* object file is opened outside of secondPass because it's simpler to create and remove it if needed from here */
        if (objFile = fopen(objFileName, "w")) {
            e2 = writeObjFileHeader(objFile, ic, dc);
            e1 = secondPass(f, objFile, symbolTable, externalTable1);
            fclose(objFile); /* secondPass is done processing object file */

            if (e1 == GOOD && e2 == GOOD) {
                e1 = generateEntriesFile(fileName, symbolTable);
                e2 = generateExternalsFile(fileName, symbolTable);
                if (e1 == GOOD && e2 == GOOD) {
                    // ok
                }else{
                    printf("error2");
                    remove(objFileName);
                    //remove(objFileName);
                    //remove(objFileName);
                }
            } else {
                printf("error 1");
                remove(objFileName); /* Delete object file, as it shouldn't be saved if an error was found. */
            }
        } else {
            printf("Error while opening file %s\n", fileName);
        }
        printf("ok?");
    }
}

/**
 * NOT TESTED!!!, no need for deep testing cuz input should be generated properly.
 *
 * @param fileName
 * @param symbolTable
 * @return
 */
enum ErrorCode generateEntriesFile(char *fileName, Symbol *symbolTable){
    char entrFileName[MAX_FILE_NAME_LEN];
    strcpy(entrFileName, fileName);
    strcat(entrFileName, ".ent");
    FILE *entrFile;
    if (entrFile = fopen(entrFileName, "w")) {
        while (symbolTable != NULL){
            if(symbolTable->attributes | ENTRY){
                if(fprintf(entrFile, "%s %.4d\n", symbolTable->label, symbolTable->address) <= 0)
                    return FILE_WRITE_ERROR;
            }
        }
        return GOOD;
    }else
        return FILE_WRITE_ERROR;
}

enum ErrorCode generateExternalsFile (char *fileName, externalTable *et){
    char externalFileName[MAX_FILE_NAME_LEN];
    strcpy(externalFileName, fileName);
    strcat(externalFileName, ".ext");
    FILE *entrFile;
    if (entrFile = fopen(externalFileName, "w")) {
        while (et != NULL){
            if(fprintf(entrFile, "%s %.4d\n", et->label, et->address) <= 0)
                return FILE_WRITE_ERROR;
        }
        return GOOD;
    }else
        return FILE_WRITE_ERROR;
}

enum ErrorCode writeObjFileHeader(FILE *pIobuf, int ic, int dc) {
    return (fprintf(pIobuf, "%d %d\n", ic, dc) > 0 )? GOOD: FILE_WRITE_ERROR;
}

enum ErrorCode firstPass(FILE *asFile, int *icOut, int *dcOut, bytesNode **dataImagePtr, Symbol **symbolTableOut) {
    int lineNumber;
    char lineStr[LINE_LENGTH + 1];
    line lineParsed;
    Symbol *symbolTable = NULL;
    byte *directiveBytes = NULL;
    int hasErrors = FALSE;
    enum ErrorCode error = GOOD; /* If encountered error */
    *icOut = 100, *dcOut = 0;
    assert(asFile != NULL);

    /* Iterate over file's lines, while counting the line number */
    /* fgetshred reads a line from asFile to lineStr */
    for (lineNumber = 0; fgetsShred(asFile, LINE_LENGTH + 1, lineStr); lineNumber++) {
        lineParsed = strToLine(lineStr);

        if (!isLineRelevant(lineParsed)) { /* skip irrelevant lines */
            continue;
        }
        else if (isLineDirective(lineParsed)) { /* process directive lines */
            if (lineParsed.label != NULL) { /* if has label, add to symbol table */
                error = addSymbol(&symbolTable, lineParsed.label, *dcOut, DATA);
                logError(error, &hasErrors, lineNumber);
            }

            /* Encode directive data */
            if (directiveBytes = directiveToBytes(lineParsed, &error))
            {
                addBytesToImage(dataImagePtr, directiveBytes);
                *dcOut += sizeof(directiveBytes);
            }
        }
        else if (!strcmp(lineParsed.label,ENTRY_MNEMONIC)) { /* process entry lines */
            continue; /* Not handled in first pass. */
        }
        else if (!strcmp(lineParsed.label,EXTERN_MNEMONIC)) { /* process extern lines */
            error = processExtern(lineParsed.head, &symbolTable, *dcOut);
            logError(error, &hasErrors, lineNumber);
        }
        else if (lineParsed.label != NULL) {/* Treat this line as an instruction, as concluded by process of elimination. */
                error = addSymbol(&symbolTable, lineParsed.label, *icOut, CODE);
                logError(error, &hasErrors, lineNumber);
                *icOut += 4;
            }
            /* Further processing of instruction line is done in second pass. This deviates then the algorithm in assignment details */
    }
    *symbolTableOut = symbolTable;
    return error;
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

char *codeToMsg(enum ErrorCode code) {
    return findErrorString(code);
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

enum ErrorCode secondPass(FILE *f, FILE *objFile, Symbol *st, externalTable  *externalTable1){
    int ic = 100, lineNo = 1;
    char lineStr[LINE_LENGTH + 1],  buf[80];
    enum ErrorCode ecTemp, ec;
    line lineParsed;
    assert(f != NULL);
    while(fgetsShred(f, LINE_LENGTH + 1, lineStr) != NULL){
        lineParsed = strToLine(lineStr);
        if(lineParsed.head.value == NULL){ /* empty line */
            lineNo++;
            continue;
        }
        ecTemp = parseInstruction(&lineParsed.head, buf, st, ic, externalTable1);
        if(ecTemp != GOOD){
            printError(ecTemp, lineNo);
            ec = GENERIC_ERROR;
        }
        if(ec == GOOD){
            printLineToFile(objFile, ic, buf);
        }
        ic += 4; /* if there is any error no output file will be generated so no need to worry if necessary to increase ic in case of a bad line */
        lineNo++;
    }
    return ec;
}
/*
 * easier to debuf buffers then files....
 * */
void printLineToBuffere(char *out, int ic, char *buf) {
    sprintf(out, "%.4d %s\n", ic, buf);
}

enum ErrorCode printLineToFile(FILE *pIobuf, int ic, char *buf) {
    char temp[80];
    printLineToBuffere(temp, ic, buf);
    return (fprintf(pIobuf, "%s", temp) > 0 )? GOOD: FILE_WRITE_ERROR;
}

/*
 * easier for testing
 * */
void makeErrStr(char *buff, enum ErrorCode ec, int lineNo){
    char *errorMsg = codeToMsg(ec);
    if (errorMsg == NULL)
        errorMsg = "Unknown error - errorcode was defined in enume ErroeCode but was not given an errorString";
    sprintf(buff, "Error at line %d: %s\n", lineNo, errorMsg);
}

void printError(enum ErrorCode ec, int lineNo){
    char buff[256];
    makeErrStr(buff, ec, lineNo);
    fprintf(stderr, "%s", buff);
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