//
// Created by Ram Ben on 7/3/2021.
//

#include "directive.h"

int directiveTypeToSize(directiveType type);

directiveType strToDirectiveType(char *name){
    if(name == NULL)
        return UNDEFINED;

    if (!strcmp(".db", name)) {
        return BYTE_TYPE;
    } else if (!strcmp(".dh", name)) {
        return HALF_WORD_TYPE;
    } else if (!strcmp(".dw", name)) {
        return WORD_TYPE;
    }else if (!strcmp(".asciz", name)) {
        return ASCII_TYPE;
    }else {
        return UNDEFINED;
    }
}

/*
 * Returns true if line's operand is a directive.
 */
int isLineDirective(line l) {
    return strToDirectiveType(l.head.value) != UNDEFINED;
}

/*
 * Convert l line (that is assumed to be a directive command) to byteArray array.
 */
byteArray directiveToBytes(line l, enum ErrorCode *errorOut) {
    char *mnemonic = l.head.value;
    char byteBuffer[LINE_LENGTH * WORD_SIZE]; /* This size will suffice the 'heaviest' case of a line like "<mnemonic> .dw 0,0,0,0..." */
    byteArray result = {NULL, 0}; /* empty result */
    int dElementSize;

    /* Convert string to byteBuffer according to the mnemonic */
    directiveType type = strToDirectiveType(mnemonic);
    switch (type) {
        case ASCII_TYPE:
            result.size = ascizParametersToBytes(l.head.next, byteBuffer, errorOut);
            break;

        /* .db, .dw, .dh are processed the same way, with only the size of each element varying */
        case BYTE_TYPE:
        case WORD_TYPE:
        case HALF_WORD_TYPE:
            dElementSize = directiveTypeToSize(type);
            result.size = lineParametersToBytes(l.head.next, byteBuffer, dElementSize);
            break;
        default:
            *errorOut = UNDEFINED_DIRECTIVE_MNEMONIC;
            return result;
    }

    /* Copy the section of the buffer that contains the generated data. */
    result.arr = (char *) malloc(sizeof(char) * result.size); /* TODO error check */
    memcpy(result.arr, byteBuffer, result.size);
    return result;
}

/*
 * Returns the size (in bytes) of each element of the directive type .db/.dh/.dw.
 * Returns 0 if type is not any of the above.
 */
int directiveTypeToSize(directiveType type) {
    switch (type) {
        case BYTE_TYPE:
            return BYTE_SIZE;
        case HALF_WORD_TYPE:
            return HALF_SIZE;
        case WORD_TYPE:
            return WORD_SIZE;
        default:
            return 0;
    }
}

/*
 * Convert linked list of nodes that represent parameters of a command, to bytes.
 * The bytes are inserted to the buffer (which is assumed to have enough room)
 * Bytes are inserted in little endian
 * Returns the number of bytes written.
 */
int lineParametersToBytes(node *head, char *buffer, int size) {
    long int res;
    int i, j, count = 0, shift;
    for (i = 0;head != NULL; head = head->next, i++) {
        assert(head->value != NULL);
        res = atol(head->value);
        if (errno != 0) {
            printf("parameter \"%s\" isn't a valid number", head->value);
            return 0;
        }
        else if (outOfBounds(res, size)) {
            printf("parameter \"%s\" is out of range\n", head->value);
            return 0;
        }

        // little endian?!?!?!
        for (j = 0, shift = 0; j < size; j++, shift += CHAR_BIT) {
            res = res >> shift;
            buffer[i * size + j] = res & 0b11111111;
            count++;
        }
    }

    return count;
}

/*
 * Copy parameter string to buffer,
 * check for illegal characters in asciz parameter.
 * head is expected to contain the string in between parenthesis in the directive line. (excluding parenthesis).
 * Returns number of bytes inserted to buffer,
 * updates 'errOut' if found an error
 */
int ascizParametersToBytes(node *head, char *buffer, enum ErrorCode *errOut) {
    /* assert that there is 1 parameter, otherwise this error should have been caught elsewhere */
    assert(head != NULL);
    assert(head->value != NULL);
    assert(head->next == NULL);

    char *str = head->value;
    int i, count = 0;

    /* Check for illegal chars */
    for (i = 0; str[i] != '\0'; i++) {
        if (!isprint(str[i])) {
            *errOut = ASCIZ_ILLEGAL_CHAR;
        }
    }
    strcpy((char *) buffer, str);
    return strlen((char *)buffer) + 1; /* +1 for Null character */
}
/*
 * Returns true if number isn't in the range of numbers that can be
 * represented by byteCount bytes.
 */
int outOfBounds(int num, int byteCount) {
    int min, max;
    switch (byteCount) {
        case 1:
            min = CHAR_MIN;
            max = CHAR_MAX;
            break;
        case 2:
            min = HALF_WORD_MIN;
            max = HALF_WORD_MAX;
            break;
        case 4:
            min = WORD_MIN;
            max = WORD_MAX;
            break;
        default:
            printf("Error, unrecognized byteArray size in 'outOfBounds' function\n");
    }
    return num > max || num < min;
}
