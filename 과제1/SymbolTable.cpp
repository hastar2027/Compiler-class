
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FILE_NAME "testdata1.txt" //파일 이름

#define STsize 30 // string table 크기
#define HTsize 100 // hash table 크기

#define isLetter(x) ( ((x) >= 'a' && (x) <='z') || ((x) >= 'A' && (x) <= 'Z') ||((x)=='_')) //문자(알파벳 및 _) 확인 함수
#define isDigit(x) ( (x) >= '0' && (x) <= '9' ) //숫자 확인 함수

typedef struct HTentry* HTpointer;
typedef struct HTentry {
    int index;            // ST 내 id index
    HTpointer next;        // 다음 id pointer
} HTentry;

typedef enum errorTypes {
    noerror, illsp, firstdigit, illid, longid, overst // 에러없음, seperator 에러(구분자가 연속하여 들어옴), 숫자로 시작, id 에러, 12자 초과 id, 오버플로우        
}ERRORtypes;

char ST[STsize];        
HTpointer HT[HTsize];   

FILE* fp; // FILE pointer               

char seperators[] = " .,;:?!\t\n"; //구분자

char input;                    // 현재 읽고 있는 character
char illid_char;                    // illid 에러에 해당하는 character
int nextid = 0, nextfree = 0;        // nextid: 현재 id, nextfree: ST 내 다음 가능한 index
ERRORtypes errr;                // 현재 에러를 담고 있는 변수

int found; // id의 이전 occurrence
int sameid; // id의 첫번째 index

//error 상황에 따라 error 내용 출력
void PrintError() {
    if (errr == noerror) return;

    printf("[Error]\t");

    switch (errr) {
    case illsp:
        printf("%c is illegal seperator\n", input);
        break;
    case longid:
        printf("%-20s\ttoo long identifier\n", ST + nextid);
        break;
    case firstdigit:
        printf("%-20s start with digit\n", ST + nextid);
        break;
    case illid:
        printf("%-20s\t%c is not allowed\n", ST + nextid, illid_char);
        break;
    case overst:
        printf("\tOVERFLOW\n");
        break;
    }

    return;
}

// 초기화 - 입력 파일 열기
void initialize() {
    fp = fopen(FILE_NAME, "r");
    if (fp == NULL) {
        printf("파일을 열지 못했습니다.\n");
        exit(1); //강제 종료
    }

    input = fgetc(fp);
}

// seperator 구분
int isSeperator(char c) {
    int i;
    int sep_len;

    sep_len = strlen(seperators);
    for (i = 0; i < sep_len; i++) {
        if (c == seperators[i])
            return 1;
    }
    return 0;
}


//구분자들은 스킵하고 다음 identifier 시작위치까지 이동
void SkipSeperators() {
    while (input != EOF && !(isLetter(input) || isDigit(input))) {
        if (!isSeperator(input)) {
            errr = illsp;
            PrintError();
        }
        input = fgetc(fp);
    }
}

//identifier 읽기
void ReadID() {
    errr = noerror;
    int invalid_flag = 0;    // 올바르지 않은 character가 존재하는지 체크
    int len = 0;        // identifier 길이

    // 첫 문자가 숫자인 경우 에러 체크 
    if (isDigit(input)) errr = firstdigit;

    // identifier 끝까지 읽기
    while (input != EOF && !isSeperator(input)) {
        // 오버플로우 체크
        if (nextfree >= STsize) {
            errr = overst;
            break;
        }

        // 올바르지 않은 character있는지 확인
        if (!isLetter(input) && !isDigit(input)) {
            // 올바르지 않은 character가 여러 개 있을 경우
            // 첫번째 것만 출력
            if (invalid_flag == 0) {
                illid_char = input;
                invalid_flag = 1;
                errr = illid;
            }
        }

        ST[nextfree++] = input;
        len++;

        input = fgetc(fp);
    }

    // id 마지막에 null(\0)문자 추가
    if (nextfree >= STsize)
        errr = overst;
    else
        ST[nextfree++] = '\0';

    // 길이 초과 에러 체크
    if (errr != overst && len > 12) errr = longid;

    // id 관련 에러 출력
    PrintError();
}




// id의 hash code 계산
int ComputeHS(int start, int end) {
    int asciisum = 0;
    for (int i = start; i < end; i++) {
        asciisum += ST[i];
    }
    return asciisum % HTsize;
}

//hash table(HT)안에 존재하는지 여부 확인 
void LookupHS(int hscode, int start, int end) {
    found = 0;
    HTpointer p = HT[hscode];
    char str[22];
    strncpy(str, ST + start, end - start);
    for (; p != NULL; p = p->next) {
        if (!strcmp(ST + p->index, str)) {
            found = 1;
            sameid = p->index;
        }    //존재하는 경우
    }//존재하지 않는 경우
}

//새로운 id HT에 추가 
void ADDHT(int hscode) {
    HTentry* hte = (HTentry*)malloc(sizeof(HTentry));
    if (hte == NULL) {
        fprintf(stderr, "malloc error\n");
        exit(1);
    }
    hte->index = nextid;
    hte->next = NULL;

    HTpointer p = HT[hscode];
    if (p == NULL) {
        HT[hscode] = hte;
    }
    else {
        hte->next = p;
        HT[hscode] = hte;
    }
}

// heading 출력
void printHeading() {
    printf("------------\t------------\n");
    printf("%s", "Index in ST");
    printf("\t");
    printf("%s", "identifier\n");
    printf("------------\t------------\n");
}

//HS table 
void PrintHStable() {
    printf("\n\n[[ HASH TABLE ]]\n\n");

    int i, cnt = 0;
    HTpointer p;
    for (i = 0; i < HTsize; i++) {
        if (HT[i] == NULL) continue;

        printf("Hash Code %2d:", i);
        for (p = HT[i]; p != NULL; p = p->next) {
            printf("%s   ", (ST + p->index));
            cnt++;
        }
        printf("\n");
    }

    printf("\n<%d characters are used in the string table>\n", nextfree);
}

int main() {
    initialize(); //파일열기
    printHeading();

    while (input != EOF) {
        // identifier를 읽어 ST에 넣기
        errr = noerror;
        SkipSeperators();
        ReadID();

        // ST에 오버플로우가 발생한 경우 종료
        if (errr == overst) {
            nextfree = nextid;
            break;
        }

        if (input != EOF && errr == noerror) {
            int hscode = ComputeHS(nextid, nextfree);
            LookupHS(hscode, nextid, nextfree);

            // 새로 삽입
            if (!found) {
                printf("%d\t\t", nextid);
                printf("%-20s", ST + nextid);
                printf("\t%s", "(entered)");
                ADDHT(hscode);
                nextid = nextfree; //id 삽입
            }
            // 이미 존재하는 경우
            else {
                printf("%d\t\t", sameid);
                printf("%-20s", ST + nextid);
                printf("\t%s", "(already existed)");
                nextfree = nextid; //id 삭제
            }
            printf("\n");
        }
        //오류가 
        else {
            nextfree = nextid; //id 삽입
        }

    }

    // 해쉬테이블 출력
    PrintHStable();

    fclose(fp);
}
