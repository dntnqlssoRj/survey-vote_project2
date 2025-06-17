#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "../include/common.h"

#define SERVER_IP   "127.0.0.1"
#define SERVER_PORT 9000

// 사용자 이름을 저장하기 위한 전역 변수
char my_username[MAX_USERNAME_LEN];

// 사용자에게 선택할 수 있는 메뉴를 출력
void print_menu();
// 설문을 생성하고 서버에 전송하는 기능
void handle_create_survey(int sockfd);
// 설문에 응답할 수 있도록 서버로부터 옵션을 받고, 사용자의 선택을 전송
void handle_respond_survey(int sockfd);
// 설문 결과를 서버에 요청하고 출력
void handle_result_survey(int sockfd);
// 투표를 생성하고 서버에 전송하는 기능
void handle_create_vote(int sockfd);
// 투표에 응답하고 서버로 전송하는 기능
void handle_respond_vote(int sockfd);
// 투표 결과를 서버에 요청하고 출력
void handle_result_vote(int sockfd);
// 설문 목록 요청 및 출력
void handle_list_survey(int sockfd);
// 투표 목록 요청 및 출력
void handle_list_vote(int sockfd);
// 특정 설문을 종료하도록 서버에 요청
void handle_close_survey(int sockfd);
// 특정 투표를 종료하도록 서버에 요청
void handle_close_vote(int sockfd);

// 클라이언트 프로그램의 진입점 - 서버에 연결하고 사용자 메뉴를 보여줌
int main() {
    int sockfd;
    struct sockaddr_in server_addr;
    char input[BUFFER_SIZE];

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket() failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family      = AF_INET;
    server_addr.sin_port        = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect() failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    printf(">> Connected to server %s:%d\n", SERVER_IP, SERVER_PORT);

    printf("Enter your username: ");
    fgets(my_username, sizeof(my_username), stdin);
    my_username[strcspn(my_username, "\n")] = '\0';
    if (strlen(my_username) == 0) {
        strncpy(my_username, "anonymous", sizeof(my_username));
    }
    printf("Welcome, %s!\n", my_username);

    while (1) {
        print_menu();
        fgets(input, sizeof(input), stdin);
        int choice = atoi(input);

        switch (choice) {
            case 1:
                handle_create_survey(sockfd);
                break;
            case 2:
                handle_respond_survey(sockfd);
                break;
            case 3:
                handle_result_survey(sockfd);
                break;
            case 4:
                handle_create_vote(sockfd);
                break;
            case 5:
                handle_respond_vote(sockfd);
                break;
            case 6:
                handle_result_vote(sockfd);
                break;
            case 7:
                handle_list_survey(sockfd);
                break;
            case 8:
                handle_list_vote(sockfd);
                break;
            case 9:
                handle_close_survey(sockfd);
                break;
            case 10:
                handle_close_vote(sockfd);
                break;
            case 0:
                close(sockfd);
                printf(">> Disconnected\n");
                return 0;
            default:
                printf("Invalid choice, try again.\n");
        }
    }

    return 0;
}

// 사용자에게 선택할 수 있는 메뉴를 출력
void print_menu() {
    printf("\n=== Menu ===\n");
    printf("1. 설문 생성\n");
    printf("2. 설문 참여\n");
    printf("3. 설문 결과 조회\n");
    printf("4. 투표 생성\n");
    printf("5. 투표 참여\n");
    printf("6. 투표 결과 조회\n");
    printf("7. 설문 목록 조회\n");
    printf("8. 투표 목록 조회\n");
    printf("9. 설문 종료\n");
    printf("10. 투표 종료\n");
    printf("0. 종료\n");
    printf("Select> ");
}

// 설문에 응답할 수 있도록 서버로부터 옵션을 받고, 사용자의 선택을 전송
void handle_respond_survey(int sockfd) {
    char buffer[BUFFER_SIZE];
    char survey_id[ID_LENGTH];
    char opt_input[BUFFER_SIZE];
    int bytes;

    // 설문 ID 입력받기
    printf("설문 ID 입력: ");
    fgets(survey_id, sizeof(survey_id), stdin);
    survey_id[strcspn(survey_id, "\n")] = '\0';
    
    // 서버에 결과 요청
    snprintf(buffer, sizeof(buffer), "%s|%s", CMD_RESULT_SURVEY, survey_id);
    send(sockfd, buffer, strlen(buffer), 0);
    // 서버로부터 옵션 목록 받기
    bytes = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
    if (bytes > 0) {
        buffer[bytes] = '\0';
        if (strncmp(buffer, "[ERROR]", 7) == 0) {
            printf("Server> %s\n", buffer);
            return;
        }
        printf("--- 설문 옵션 ---\n%s\n", buffer);
    } else {
        printf("Failed to receive data from server.\n");
        return;
    }

    // 사용자 응답 입력받기
    printf("선택 항목 번호 입력 (예: 1,2): ");
    fgets(opt_input, sizeof(opt_input), stdin);
    opt_input[strcspn(opt_input, "\n")] = '\0';

    // 서버에 응답 전송
    snprintf(buffer, sizeof(buffer), "%s|%s|%s|%s",
             CMD_RESPOND_SURVEY, survey_id, opt_input, my_username);
    send(sockfd, buffer, strlen(buffer), 0);

    // 서버로부터 처리 결과 받기
    bytes = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
    if (bytes > 0) {
        buffer[bytes] = '\0';
        printf("Server> %s\n", buffer);
    }
}

// 투표에 응답하고 서버로 전송하는 기능
void handle_respond_vote(int sockfd) {
    char buffer[BUFFER_SIZE];
    char vote_id[ID_LENGTH];
    char opt_input[BUFFER_SIZE];
    int bytes;

    // 투표 ID 입력받기
    printf("투표 ID 입력: ");
    fgets(vote_id, sizeof(vote_id), stdin);
    vote_id[strcspn(vote_id, "\n")] = '\0';

    // 서버에 결과 요청
    snprintf(buffer, sizeof(buffer), "%s|%s", CMD_RESULT_VOTE, vote_id);
    send(sockfd, buffer, strlen(buffer), 0);
    // 서버로부터 옵션 목록 받기
    bytes = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
    if (bytes > 0) {
        buffer[bytes] = '\0';
        if (strncmp(buffer, "[ERROR]", 7) == 0) {
            printf("Server> %s\n", buffer);
            return;
        }
        printf("--- 투표 옵션 ---\n%s\n", buffer);
    } else {
        printf("Failed to receive data from server.\n");
        return;
    }

    // 사용자 응답 입력받기
    printf("선택할 보기의 번호를 하나만 입력하세요: ");
    fgets(opt_input, sizeof(opt_input), stdin);
    opt_input[strcspn(opt_input, "\n")] = '\0';

    // 유효성 검사
    if(strlen(opt_input) == 0){
        printf("Error: 번호를 입력해주세요.\n");
        return;
    }
    for (int i = 0; i < strlen(opt_input); i++) {
        if (opt_input[i] < '0' || opt_input[i] > '9') {
            printf("Error: 숫자만 입력해주세요.\n");
            return;
        }
    }

    // 서버에 응답 전송
    snprintf(buffer, sizeof(buffer), "%s|%s|%s|%s",
             CMD_RESPOND_VOTE, vote_id, opt_input, my_username);
    send(sockfd, buffer, strlen(buffer), 0);

    // 서버로부터 처리 결과 받기
    bytes = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
    if (bytes > 0) {
        buffer[bytes] = '\0';
        printf("Server> %s\n", buffer);
    }
}

// 특정 설문을 종료하도록 서버에 요청
void handle_close_survey(int sockfd) {
    char buffer[BUFFER_SIZE];
    char survey_id[ID_LENGTH];

    printf("종료할 설문의 ID를 입력하세요: ");
    fgets(survey_id, sizeof(survey_id), stdin);
    survey_id[strcspn(survey_id, "\n")] = '\0';

    snprintf(buffer, sizeof(buffer), "%s|%s", CMD_CLOSE_SURVEY, survey_id);
    send(sockfd, buffer, strlen(buffer), 0);

    int bytes = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
    if (bytes > 0) {
        buffer[bytes] = '\0';
        printf("Server> %s\n", buffer);
    }
}

// 특정 투표를 종료하도록 서버에 요청
void handle_close_vote(int sockfd) {
    char buffer[BUFFER_SIZE];
    char vote_id[ID_LENGTH];

    printf("종료할 투표의 ID를 입력하세요: ");
    fgets(vote_id, sizeof(vote_id), stdin);
    vote_id[strcspn(vote_id, "\n")] = '\0';

    snprintf(buffer, sizeof(buffer), "%s|%s", CMD_CLOSE_VOTE, vote_id);
    send(sockfd, buffer, strlen(buffer), 0);

    int bytes = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
    if (bytes > 0) {
        buffer[bytes] = '\0';
        printf("Server> %s\n", buffer);
    }
}

// 설문을 생성하고 서버에 전송하는 기능
void handle_create_survey(int sockfd) {
    char buffer[BUFFER_SIZE];
    char question[MAX_QUESTION_LEN];
    char opt_input[BUFFER_SIZE];
    char options_csv[MAX_OPTIONS * (MAX_OPTION_LEN + 1)] = "";
    int n_opts = 0;

    printf("설문 질문 입력: ");
    fgets(question, sizeof(question), stdin);
    question[strcspn(question, "\n")] = '\0';

    do {
        printf("보기 옵션 개수 입력 (2~%d): ", MAX_OPTIONS);
        fgets(opt_input, sizeof(opt_input), stdin);
        n_opts = atoi(opt_input);
    } while (n_opts < 2 || n_opts > MAX_OPTIONS);

    for (int i = 0; i < n_opts; i++) {
        printf("옵션 %d 입력: ", i + 1);
        fgets(opt_input, sizeof(opt_input), stdin);
        opt_input[strcspn(opt_input, "\n")] = '\0';
        strcat(options_csv, opt_input);
        if (i < n_opts - 1) {
            strcat(options_csv, ",");
        }
    }

    snprintf(buffer, sizeof(buffer), "%s|%s|%s",
             CMD_CREATE_SURVEY, question, options_csv);
    send(sockfd, buffer, strlen(buffer), 0);

    int bytes = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
    if (bytes > 0) {
        buffer[bytes] = '\0';
        printf("Server> %s\n", buffer);
    }
}

// 설문 결과를 서버에 요청하고 출력
void handle_result_survey(int sockfd) {
    char buffer[BUFFER_SIZE];
    char survey_id[ID_LENGTH];

    printf("설문 ID 입력: ");
    fgets(survey_id, sizeof(survey_id), stdin);
    survey_id[strcspn(survey_id, "\n")] = '\0';

    snprintf(buffer, sizeof(buffer), "%s|%s",
             CMD_RESULT_SURVEY, survey_id);
    send(sockfd, buffer, strlen(buffer), 0);

    int bytes = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
    if (bytes > 0) {
        buffer[bytes] = '\0';
        printf("Server> %s\n", buffer);
    }
}

// 투표를 생성하고 서버에 전송하는 기능
void handle_create_vote(int sockfd) {
    char buffer[BUFFER_SIZE];
    char title[MAX_QUESTION_LEN];
    char opt_input[BUFFER_SIZE];
    char options_csv[MAX_OPTIONS * (MAX_OPTION_LEN + 1)] = "";
    int n_opts = 0;

    printf("투표 제목 입력: ");
    fgets(title, sizeof(title), stdin);
    title[strcspn(title, "\n")] = '\0';

    do {
        printf("보기 옵션 개수 입력 (2~%d): ", MAX_OPTIONS);
        fgets(opt_input, sizeof(opt_input), stdin);
        n_opts = atoi(opt_input);
    } while (n_opts < 2 || n_opts > MAX_OPTIONS);

    for (int i = 0; i < n_opts; i++) {
        printf("옵션 %d 입력: ", i + 1);
        fgets(opt_input, sizeof(opt_input), stdin);
        opt_input[strcspn(opt_input, "\n")] = '\0';
        strcat(options_csv, opt_input);
        if (i < n_opts - 1) {
            strcat(options_csv, ",");
        }
    }

    snprintf(buffer, sizeof(buffer), "%s|%s|%s",
             CMD_CREATE_VOTE, title, options_csv);
    send(sockfd, buffer, strlen(buffer), 0);

    int bytes = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
    if (bytes > 0) {
        buffer[bytes] = '\0';
        printf("Server> %s\n", buffer);
    }
}

// 투표 결과를 서버에 요청하고 출력
void handle_result_vote(int sockfd) {
    char buffer[BUFFER_SIZE];
    char vote_id[ID_LENGTH];

    printf("투표 ID 입력: ");
    fgets(vote_id, sizeof(vote_id), stdin);
    vote_id[strcspn(vote_id, "\n")] = '\0';

    snprintf(buffer, sizeof(buffer), "%s|%s",
             CMD_RESULT_VOTE, vote_id);
    send(sockfd, buffer, strlen(buffer), 0);

    int bytes = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
    if (bytes > 0) {
        buffer[bytes] = '\0';
        printf("Server> %s\n", buffer);
    }
}

// 설문 목록 요청 및 출력
void handle_list_survey(int sockfd) {
    char buffer[BUFFER_SIZE];
    snprintf(buffer, sizeof(buffer), "%s", CMD_LIST_SURVEY);
    send(sockfd, buffer, strlen(buffer), 0);
    int bytes = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
    if (bytes > 0) {
        buffer[bytes] = '\0';
        printf("--- 설문 목록 ---\n%s", buffer);
    }
}

// 투표 목록 요청 및 출력
void handle_list_vote(int sockfd) {
    char buffer[BUFFER_SIZE];
    snprintf(buffer, sizeof(buffer), "%s", CMD_LIST_VOTE);
    send(sockfd, buffer, strlen(buffer), 0);
    int bytes = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
    if (bytes > 0) {
        buffer[bytes] = '\0';
        printf("--- 투표 목록 ---\n%s", buffer);
    }
}