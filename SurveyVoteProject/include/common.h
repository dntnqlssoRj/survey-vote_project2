// common.h: 설문조사 및 투표 시스템에서 사용하는 공통 구조체와 명령어 정의
#ifndef SURVEY_VOTE_COMMON_H
#define SURVEY_VOTE_COMMON_H

#include <stddef.h>
#include <string.h>

// 설문 질문 또는 투표 제목의 최대 글자 수
#define MAX_QUESTION_LEN 256

// 보기(option) 항목의 최대 글자 수
#define MAX_OPTION_LEN   64

// 설문이나 투표에서 최대 몇 개의 보기 항목을 제공할 수 있는지 정의
#define MAX_OPTIONS      5

// 클라이언트-서버 간 데이터 전송 시 사용할 버퍼 크기
#define BUFFER_SIZE      1024

// 설문/투표에 부여되는 ID 문자열의 최대 길이
#define ID_LENGTH        64


// 사용자 이름의 최대 글자 수
#define MAX_USERNAME_LEN 32

// 하나의 설문이나 투표에서 참여할 수 있는 최대 인원 수
#define MAX_VOTERS       100


// 설문 또는 투표가 현재 진행 중인지, 종료되었는지를 나타냄
typedef enum {
    STATUS_ACTIVE, // 진행중
    STATUS_CLOSED  // 종료됨
} ItemStatus;


// Survey 구조체: 하나의 설문에 대한 모든 정보를 담는 구조체
typedef struct Survey {
    char id[ID_LENGTH];
    char question[MAX_QUESTION_LEN];
    int  option_count;
    char options[MAX_OPTIONS][MAX_OPTION_LEN];
    int  votes[MAX_OPTIONS];
    ItemStatus status;
    char voters[MAX_VOTERS][MAX_USERNAME_LEN]; // 이 설문에 참여한 사용자들의 이름 목록
    int voter_count;                           // 현재까지 설문에 참여한 인원 수
    struct Survey* next;
} Survey;

// Vote 구조체: 하나의 투표에 대한 모든 정보를 담는 구조체
typedef struct Vote {
    char id[ID_LENGTH];
    char title[MAX_QUESTION_LEN];
    int  option_count;
    char options[MAX_OPTIONS][MAX_OPTION_LEN];
    int  votes[MAX_OPTIONS];
    ItemStatus status;
    char voters[MAX_VOTERS][MAX_USERNAME_LEN]; // 이 설문에 참여한 사용자들의 이름 목록
    int voter_count;                           // 현재까지 설문에 참여한 인원 수
    struct Vote* next;
} Vote;


// 클라이언트와 서버가 통신할 때 사용하는 명령어 문자열

// 설문 관련 명령어
#define CMD_CREATE_SURVEY   "CREATE_SURVEY"
#define CMD_RESPOND_SURVEY  "RESPOND_SURVEY"
#define CMD_RESULT_SURVEY   "RESULT_SURVEY"
#define CMD_LIST_SURVEY     "LIST_SURVEY"
#define CMD_CLOSE_SURVEY    "CLOSE_SURVEY"

// 투표 관련 명령어
#define CMD_CREATE_VOTE     "CREATE_VOTE"
#define CMD_RESPOND_VOTE    "RESPOND_VOTE"
#define CMD_RESULT_VOTE     "RESULT_VOTE"
#define CMD_LIST_VOTE       "LIST_VOTE"
#define CMD_CLOSE_VOTE      "CLOSE_VOTE"


#endif  // SURVEY_VOTE_COMMON_H