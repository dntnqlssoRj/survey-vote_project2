#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <ctype.h>
#include "../include/common.h"
#include <time.h>
#include <sys/stat.h>
#include <dirent.h>

#define SERVER_PORT 9000
#define BACKLOG     5

void* handle_client(void* arg);
void create_survey_handler(int sockfd, char* msg);
void respond_survey_handler(int sockfd, char* msg);
void result_survey_handler(int sockfd, char* msg);
void close_survey_handler(int sockfd, char* msg);
void list_survey_handler(int sockfd, char* msg);
void create_vote_handler(int sockfd, char* msg);
void respond_vote_handler(int sockfd, char* msg);
void result_vote_handler(int sockfd, char* msg);
void close_vote_handler(int sockfd, char* msg);
void list_vote_handler(int sockfd, char* msg);
void load_surveys();
void load_votes();
void save_survey_to_file(Survey* survey);
void save_vote_to_file(Vote* vote);
void slugify(const char* input, char* output, size_t max_len);
int id_exists(const char* id, const char* type);

// 전역 변수
static Survey* survey_head = NULL;
static Vote* vote_head = NULL;
pthread_mutex_t data_lock;

// 문자열을 소문자 및 하이픈(-)으로 구성된 ID로 변환
void slugify(const char* input, char* output, size_t max_len) {
    size_t i = 0, j = 0;
    int last_char_is_hyphen = 0;

    while (input[i] != '\0' && j < max_len - 1) {
        if (isalnum(input[i])) {
            output[j++] = tolower(input[i]);
            last_char_is_hyphen = 0;
        } else if (isspace(input[i])) {
            if (!last_char_is_hyphen) {
                output[j++] = '-';
                last_char_is_hyphen = 1;
            }
        }
        i++;
    }
    if (j > 0 && output[j - 1] == '-') {
        output[j - 1] = '\0';
    } else {
        output[j] = '\0';
    }
}

// ID에 해당하는 파일이 존재하는지 확인
int id_exists(const char* id, const char* type) {
    char filename[256];
    snprintf(filename, sizeof(filename), "data/%s/%s.txt", type, id);
    if (access(filename, F_OK) == 0) {
        return 1;
    }
    return 0;
}

// 서버 프로그램의 진입점 - 클라이언트 요청을 기다리고 각 요청을 새 스레드로 처리
int main() {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    pthread_t tid;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket() failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family      = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port        = htons(SERVER_PORT);

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind() failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, BACKLOG) < 0) {
        perror("listen() failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (pthread_mutex_init(&data_lock, NULL) != 0) {
        perror("mutex init failed");
        exit(EXIT_FAILURE);
    }
    
    mkdir("data", 0755);
    mkdir("data/survey", 0755);
    mkdir("data/vote", 0755);

    load_surveys();
    load_votes();
    printf(">> Server listening on port %d\n", SERVER_PORT);

    while (1) {
        client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &addr_len);
        if (client_fd < 0) {
            perror("accept() failed");
            continue;
        }
        printf(">> Client connected: %s:%d\n",
               inet_ntoa(client_addr.sin_addr),
               ntohs(client_addr.sin_port));

        // --- [수정] --- 스레드에 안전한 방식으로 인자 전달 ---
        int* p_client_fd = malloc(sizeof(int));
        *p_client_fd = client_fd;
        if (pthread_create(&tid, NULL, handle_client, p_client_fd) != 0) {
            perror("pthread_create() failed");
            close(client_fd);
            free(p_client_fd);
        }
        // --- 수정 끝 ---
        pthread_detach(tid);
    }

    pthread_mutex_destroy(&data_lock);
    close(server_fd);
    return 0;
}

void* handle_client(void* arg)
{
    // --- [수정] --- 스레드 시작 시 인자를 안전하게 복사하고 메모리 해제 ---
    int sockfd = *(int*)arg;
    free(arg);
    // --- 수정 끝 ---

    char buffer[BUFFER_SIZE];
    int bytes;

    while ((bytes = recv(sockfd, buffer, sizeof(buffer) - 1, 0)) > 0) {
        // 클라이언트로부터 메시지 수신
        buffer[bytes] = '\0';
        
        // 원본 메시지를 변경하지 않기 위해 복사본 생성
        char msg_copy[BUFFER_SIZE];
        strncpy(msg_copy, buffer, sizeof(msg_copy));

        // 설문 생성 요청 처리
        if (strncmp(msg_copy, CMD_CREATE_SURVEY, strlen(CMD_CREATE_SURVEY)) == 0) {
            create_survey_handler(sockfd, msg_copy);
        }
        // 설문 응답 요청 처리
        else if (strncmp(msg_copy, CMD_RESPOND_SURVEY, strlen(CMD_RESPOND_SURVEY)) == 0) {
            respond_survey_handler(sockfd, msg_copy);
        }
        // 설문 결과 요청 처리
        else if (strncmp(msg_copy, CMD_RESULT_SURVEY, strlen(CMD_RESULT_SURVEY)) == 0) {
            result_survey_handler(sockfd, msg_copy);
        }
        // 설문 종료 요청 처리
        else if (strncmp(msg_copy, CMD_CLOSE_SURVEY, strlen(CMD_CLOSE_SURVEY)) == 0) {
            close_survey_handler(sockfd, msg_copy);
        }
        // 투표 생성 요청 처리
        else if (strncmp(msg_copy, CMD_CREATE_VOTE, strlen(CMD_CREATE_VOTE)) == 0) {
            create_vote_handler(sockfd, msg_copy);
        }
        // 투표 응답 요청 처리
        else if (strncmp(msg_copy, CMD_RESPOND_VOTE, strlen(CMD_RESPOND_VOTE)) == 0) {
            respond_vote_handler(sockfd, msg_copy);
        }
        // 투표 결과 요청 처리
        else if (strncmp(msg_copy, CMD_RESULT_VOTE, strlen(CMD_RESULT_VOTE)) == 0) {
            result_vote_handler(sockfd, msg_copy);
        }
        // 투표 종료 요청 처리
        else if (strncmp(msg_copy, CMD_CLOSE_VOTE, strlen(CMD_CLOSE_VOTE)) == 0) {
            close_vote_handler(sockfd, msg_copy);
        }
        // 설문 목록 요청 처리
        else if (strncmp(msg_copy, CMD_LIST_SURVEY, strlen(CMD_LIST_SURVEY)) == 0) {
            list_survey_handler(sockfd, msg_copy);
        }
        // 투표 목록 요청 처리
        else if (strncmp(msg_copy, CMD_LIST_VOTE, strlen(CMD_LIST_VOTE)) == 0) {
            list_vote_handler(sockfd, msg_copy);
        }
        else {
            send(sockfd, "[ERROR] Unknown command", strlen("[ERROR] Unknown command"), 0);
        }
    }

    printf(">> Client disconnected\n");
    close(sockfd);
    return NULL;
}

void save_survey_to_file(Survey* survey) {
    // 설문 정보를 파일로 저장
    char filename[256];
    snprintf(filename, sizeof(filename), "data/survey/%s.txt", survey->id);
    FILE* fp = fopen(filename, "w");
    if (fp) {
        fprintf(fp, "%s\n", survey->question);
        fprintf(fp, "%d\n", survey->status);
        for (int i = 0; i < survey->option_count; i++) {
            fprintf(fp, "%s:%d\n", survey->options[i], survey->votes[i]);
        }
        fprintf(fp, "---VOTERS---\n");
        for (int i = 0; i < survey->voter_count; i++) {
            fprintf(fp, "%s\n", survey->voters[i]);
        }
        fclose(fp);
    }
}

void save_vote_to_file(Vote* vote) {
    // 투표 정보를 파일로 저장
    char filename[256];
    snprintf(filename, sizeof(filename), "data/vote/%s.txt", vote->id);
    FILE* fp = fopen(filename, "w");
    if (fp) {
        fprintf(fp, "%s\n", vote->title);
        fprintf(fp, "%d\n", vote->status);
        for (int i = 0; i < vote->option_count; i++) {
            fprintf(fp, "%s:%d\n", vote->options[i], vote->votes[i]);
        }
        fprintf(fp, "---VOTERS---\n");
        for (int i = 0; i < vote->voter_count; i++) {
            fprintf(fp, "%s\n", vote->voters[i]);
        }
        fclose(fp);
    }
}

void load_surveys() {
    // 저장된 설문 파일들을 읽어서 메모리로 복구
    DIR* d = opendir("data/survey");
    if (!d) return;
    struct dirent* e;
    while ((e = readdir(d)) != NULL) {
        if (e->d_type == DT_REG && strstr(e->d_name, ".txt")) {
            char path[256];
            snprintf(path, sizeof(path), "data/survey/%s", e->d_name);
            FILE* f = fopen(path, "r");
            if (!f) continue;

            Survey* node = malloc(sizeof(Survey));
            memset(node, 0, sizeof(Survey));

            strncpy(node->id, e->d_name, strlen(e->d_name) - 4);
            node->id[strlen(e->d_name) - 4] = '\0';
            
            fgets(node->question, sizeof(node->question), f);
            node->question[strcspn(node->question, "\n")] = '\0';
            
            char status_line[16];
            if (fgets(status_line, sizeof(status_line), f)) {
                node->status = (ItemStatus)atoi(status_line);
            } else {
                node->status = STATUS_ACTIVE;
            }
            
            char line[BUFFER_SIZE];
            int idx = 0;
            int parsing_options = 1;

            while (fgets(line, sizeof(line), f)) {
                line[strcspn(line, "\n")] = 0;
                if (strlen(line) == 0) continue;

                if (strcmp(line, "---VOTERS---") == 0) {
                    parsing_options = 0;
                    continue;
                }

                if (parsing_options) {
                    if (idx < MAX_OPTIONS) {
                        char* vote_str = strrchr(line, ':');
                        if (vote_str) {
                            *vote_str = '\0';
                            node->votes[idx] = atoi(vote_str + 1);
                        } else {
                            node->votes[idx] = 0;
                        }
                        strncpy(node->options[idx], line, MAX_OPTION_LEN);
                        idx++;
                    }
                } else {
                    if (node->voter_count < MAX_VOTERS) {
                        strncpy(node->voters[node->voter_count], line, MAX_USERNAME_LEN);
                        node->voter_count++;
                    }
                }
            }
            node->option_count = idx;
            
            fclose(f);
            node->next = survey_head;
            survey_head = node;
        }
    }
    closedir(d);
}

void load_votes() {
    // 저장된 투표 파일들을 읽어서 메모리로 복구
    DIR* d = opendir("data/vote");
    if (!d) return;
    struct dirent* e;
    while ((e = readdir(d)) != NULL) {
        if (e->d_type == DT_REG && strstr(e->d_name, ".txt")) {
            char path[256];
            snprintf(path, sizeof(path), "data/vote/%s", e->d_name);
            FILE* f = fopen(path, "r");
            if (!f) continue;
            
            Vote* node = malloc(sizeof(Vote));
            memset(node, 0, sizeof(Vote));

            strncpy(node->id, e->d_name, strlen(e->d_name) - 4);
            node->id[strlen(e->d_name) - 4] = '\0';
            
            fgets(node->title, sizeof(node->title), f);
            node->title[strcspn(node->title, "\n")] = '\0';
            
            char status_line[16];
            if (fgets(status_line, sizeof(status_line), f)) {
                node->status = (ItemStatus)atoi(status_line);
            } else {
                node->status = STATUS_ACTIVE;
            }
            
            char line[BUFFER_SIZE];
            int idx = 0;
            int parsing_options = 1;

            while (fgets(line, sizeof(line), f)) {
                line[strcspn(line, "\n")] = 0;
                if (strlen(line) == 0) continue;

                if (strcmp(line, "---VOTERS---") == 0) {
                    parsing_options = 0;
                    continue;
                }

                if (parsing_options) {
                    if (idx < MAX_OPTIONS) {
                        char* vote_str = strrchr(line, ':');
                        if (vote_str) {
                            *vote_str = '\0';
                            node->votes[idx] = atoi(vote_str + 1);
                        } else {
                            node->votes[idx] = 0;
                        }
                        strncpy(node->options[idx], line, MAX_OPTION_LEN);
                        idx++;
                    }
                } else {
                     if (node->voter_count < MAX_VOTERS) {
                        strncpy(node->voters[node->voter_count], line, MAX_USERNAME_LEN);
                        node->voter_count++;
                    }
                }
            }
            node->option_count = idx;
            
            fclose(f);
            node->next = vote_head;
            vote_head = node;
        }
    }
    closedir(d);
}

// - create_survey_handler: 설문 생성 요청 처리
void create_survey_handler(int sockfd, char* msg)
{
    char resp[BUFFER_SIZE];
    char* saveptr;
    
    pthread_mutex_lock(&data_lock);

    strtok_r(msg, "|", &saveptr);
    char* question = strtok_r(NULL, "|", &saveptr);
    char* opts_csv = strtok_r(NULL, "|", &saveptr);

    if (question == NULL || opts_csv == NULL) {
        pthread_mutex_unlock(&data_lock);
        snprintf(resp, sizeof(resp), "[ERROR] Invalid format for CREATE_SURVEY");
        send(sockfd, resp, strlen(resp), 0);
        return;
    }

    char base_id[ID_LENGTH];
    char final_id[ID_LENGTH];
    slugify(question, base_id, sizeof(base_id));
    
    if (strlen(base_id) == 0) {
        strncpy(base_id, "survey", sizeof(base_id));
    }

    strncpy(final_id, base_id, sizeof(final_id));
    int suffix = 2;
    while (id_exists(final_id, "survey")) {
        snprintf(final_id, sizeof(final_id), "%s-%d", base_id, suffix++);
    }

    Survey* node = malloc(sizeof(Survey));
    memset(node, 0, sizeof(Survey));
    strncpy(node->id, final_id, ID_LENGTH);
    strncpy(node->question, question, MAX_QUESTION_LEN);
    node->status = STATUS_ACTIVE;
    node->voter_count = 0;
    
    int i = 0;
    char* saveptr_opts;
    char* opt = strtok_r(opts_csv, ",", &saveptr_opts);
    while(opt && i < MAX_OPTIONS) {
        strncpy(node->options[i], opt, MAX_OPTION_LEN);
        node->votes[i] = 0;
        i++;
        opt = strtok_r(NULL, ",", &saveptr_opts);
    }
    node->option_count = i;
    
    save_survey_to_file(node);

    node->next = survey_head;
    survey_head = node;

    pthread_mutex_unlock(&data_lock);

    snprintf(resp, sizeof(resp), "[OK] Survey created with ID: %s", final_id);
    send(sockfd, resp, strlen(resp), 0);
}

// - create_vote_handler: 투표 생성 요청 처리
void create_vote_handler(int sockfd, char* msg) {
    char resp[BUFFER_SIZE];
    char* saveptr;
    
    pthread_mutex_lock(&data_lock);
    
    strtok_r(msg, "|", &saveptr);
    char* title = strtok_r(NULL, "|", &saveptr);
    char* opts_csv = strtok_r(NULL, "|", &saveptr);

    if (title == NULL || opts_csv == NULL) {
        pthread_mutex_unlock(&data_lock);
        snprintf(resp, sizeof(resp), "[ERROR] Invalid format for CREATE_VOTE");
        send(sockfd, resp, strlen(resp), 0);
        return;
    }

    char base_id[ID_LENGTH];
    char final_id[ID_LENGTH];
    slugify(title, base_id, sizeof(base_id));
    
    if (strlen(base_id) == 0) {
        strncpy(base_id, "vote", sizeof(base_id));
    }
    
    strncpy(final_id, base_id, sizeof(final_id));
    int suffix = 2;
    while (id_exists(final_id, "vote")) {
        snprintf(final_id, sizeof(final_id), "%s-%d", base_id, suffix++);
    }
    
    Vote* node = malloc(sizeof(Vote));
    memset(node, 0, sizeof(Vote));
    strncpy(node->id, final_id, ID_LENGTH);
    strncpy(node->title, title, MAX_QUESTION_LEN);
    node->status = STATUS_ACTIVE;
    node->voter_count = 0;

    int i = 0;
    char* saveptr_opts;
    char* opt = strtok_r(opts_csv, ",", &saveptr_opts);
    while(opt && i < MAX_OPTIONS) {
        strncpy(node->options[i], opt, MAX_OPTION_LEN);
        node->votes[i] = 0;
        i++;
        opt = strtok_r(NULL, ",", &saveptr_opts);
    }
    node->option_count = i;

    save_vote_to_file(node);

    node->next = vote_head;
    vote_head = node;

    pthread_mutex_unlock(&data_lock);

    snprintf(resp, sizeof(resp), "[OK] Vote created with ID: %s", final_id);
    send(sockfd, resp, strlen(resp), 0);
}

// - respond_survey_handler: 설문 응답 요청 처리
void respond_survey_handler(int sockfd, char* msg) 
{
    char* saveptr;
    pthread_mutex_lock(&data_lock);

    strtok_r(msg, "|", &saveptr);
    char* id = strtok_r(NULL, "|", &saveptr);
    char* opts_csv = strtok_r(NULL, "|", &saveptr);
    char* username = strtok_r(NULL, "|", &saveptr);

    if (id == NULL || opts_csv == NULL || username == NULL) {
        pthread_mutex_unlock(&data_lock);
        send(sockfd, "[ERROR] Invalid format for RESPOND_SURVEY", strlen("[ERROR] Invalid format for RESPOND_SURVEY"), 0);
        return;
    }
    
    Survey* cur = survey_head;
    while (cur && strcmp(cur->id, id) != 0) {
        cur = cur->next;
    }
    if (!cur) {
        pthread_mutex_unlock(&data_lock);
        send(sockfd, "[ERROR] Survey not found", strlen("[ERROR] Survey not found"), 0);
        return;
    }

    if (cur->status == STATUS_CLOSED) {
        pthread_mutex_unlock(&data_lock);
        send(sockfd, "[ERROR] This survey is closed.", strlen("[ERROR] This survey is closed."), 0);
        return;
    }

    for (int i = 0; i < cur->voter_count; i++) {
        if (strcmp(cur->voters[i], username) == 0) {
            pthread_mutex_unlock(&data_lock);
            send(sockfd, "[ERROR] You have already participated in this survey.", strlen("[ERROR] You have already participated in this survey."), 0);
            return;
        }
    }

    if (cur->voter_count >= MAX_VOTERS) {
        pthread_mutex_unlock(&data_lock);
        send(sockfd, "[ERROR] This survey has reached its maximum number of participants.", strlen("[ERROR] This survey has reached its maximum number of participants."), 0);
        return;
    }

    char* saveptr_opts;
    char* token = strtok_r(opts_csv, ",", &saveptr_opts);
    while (token) {
        int idx = atoi(token) - 1;
        if (idx >= 0 && idx < cur->option_count) {
            cur->votes[idx]++;
        }
        token = strtok_r(NULL, ",", &saveptr_opts);
    }

    strncpy(cur->voters[cur->voter_count], username, MAX_USERNAME_LEN);
    cur->voter_count++;

    save_survey_to_file(cur);
    pthread_mutex_unlock(&data_lock);
    
    send(sockfd, "[OK] Your response has been recorded.", strlen("[OK] Your response has been recorded."), 0);
}


// - respond_vote_handler: 투표 응답 요청 처리
void respond_vote_handler(int sockfd, char* msg) {
    char* saveptr;
    pthread_mutex_lock(&data_lock);

    strtok_r(msg, "|", &saveptr);
    char* id = strtok_r(NULL, "|", &saveptr);
    char* opt_str = strtok_r(NULL, "|", &saveptr);
    char* username = strtok_r(NULL, "|", &saveptr);

    if (id == NULL || opt_str == NULL || username == NULL) {
        pthread_mutex_unlock(&data_lock);
        send(sockfd, "[ERROR] Invalid format for RESPOND_VOTE", strlen("[ERROR] Invalid format for RESPOND_VOTE"), 0);
        return;
    }

    Vote* cur = vote_head;
    while (cur && strcmp(cur->id, id) != 0) {
        cur = cur->next;
    }
    if (!cur) {
        pthread_mutex_unlock(&data_lock);
        send(sockfd, "[ERROR] Vote not found", strlen("[ERROR] Vote not found"), 0);
        return;
    }

    if (cur->status == STATUS_CLOSED) {
        pthread_mutex_unlock(&data_lock);
        send(sockfd, "[ERROR] This vote is closed.", strlen("[ERROR] This vote is closed."), 0);
        return;
    }

    for (int i = 0; i < cur->voter_count; i++) {
        if (strcmp(cur->voters[i], username) == 0) {
            pthread_mutex_unlock(&data_lock);
            send(sockfd, "[ERROR] You have already voted on this item.", strlen("[ERROR] You have already voted on this item."), 0);
            return;
        }
    }
    
    if (cur->voter_count >= MAX_VOTERS) {
        pthread_mutex_unlock(&data_lock);
        send(sockfd, "[ERROR] This vote has reached its maximum number of participants.", strlen("[ERROR] This vote has reached its maximum number of participants."), 0);
        return;
    }

    int idx = atoi(opt_str) - 1;
    if (idx >= 0 && idx < cur->option_count) {
        cur->votes[idx]++;
    }

    strncpy(cur->voters[cur->voter_count], username, MAX_USERNAME_LEN);
    cur->voter_count++;

    save_vote_to_file(cur);
    pthread_mutex_unlock(&data_lock);

    send(sockfd, "[OK] Your vote has been recorded.", strlen("[OK] Your vote has been recorded."), 0);
}

// - close_survey_handler: 설문 종료 요청 처리
void close_survey_handler(int sockfd, char* msg) {
    char* saveptr;
    pthread_mutex_lock(&data_lock);
    strtok_r(msg, "|", &saveptr);
    char* id = strtok_r(NULL, "|", &saveptr);
    if (id == NULL) {
        pthread_mutex_unlock(&data_lock);
        send(sockfd, "[ERROR] Invalid format for CLOSE_SURVEY", strlen("[ERROR] Invalid format for CLOSE_SURVEY"), 0);
        return;
    }
    Survey* cur = survey_head;
    while (cur && strcmp(cur->id, id) != 0) {
        cur = cur->next;
    }
    if (!cur) {
        pthread_mutex_unlock(&data_lock);
        send(sockfd, "[ERROR] Survey not found", strlen("[ERROR] Survey not found"), 0);
        return;
    }
    cur->status = STATUS_CLOSED;
    save_survey_to_file(cur);
    pthread_mutex_unlock(&data_lock);
    char resp[BUFFER_SIZE];
    snprintf(resp, sizeof(resp), "[OK] Survey %s is now closed.", id);
    send(sockfd, resp, strlen(resp), 0);
}

// - close_vote_handler: 투표 종료 요청 처리
void close_vote_handler(int sockfd, char* msg) {
    char* saveptr;
    pthread_mutex_lock(&data_lock);
    strtok_r(msg, "|", &saveptr);
    char* id = strtok_r(NULL, "|", &saveptr);
    if (id == NULL) {
        pthread_mutex_unlock(&data_lock);
        send(sockfd, "[ERROR] Invalid format for CLOSE_VOTE", strlen("[ERROR] Invalid format for CLOSE_VOTE"), 0);
        return;
    }
    Vote* cur = vote_head;
    while (cur && strcmp(cur->id, id) != 0) {
        cur = cur->next;
    }
    if (!cur) {
        pthread_mutex_unlock(&data_lock);
        send(sockfd, "[ERROR] Vote not found", strlen("[ERROR] Vote not found"), 0);
        return;
    }
    cur->status = STATUS_CLOSED;
    save_vote_to_file(cur);
    pthread_mutex_unlock(&data_lock);
    char resp[BUFFER_SIZE];
    snprintf(resp, sizeof(resp), "[OK] Vote %s is now closed.", id);
    send(sockfd, resp, strlen(resp), 0);
}

// - list_survey_handler: 설문 목록 요청 처리
void list_survey_handler(int sockfd, char* msg) {
    char resp[BUFFER_SIZE] = {0};
    int offset = 0;
    pthread_mutex_lock(&data_lock);
    Survey* cur = survey_head;
    while (cur) {
        const char* status_str = (cur->status == STATUS_ACTIVE) ? "Active" : "Closed";
        offset += snprintf(resp + offset, sizeof(resp) - offset,
                           "[%s] ID: %s, Question: %s\n", status_str, cur->id, cur->question);
        cur = cur->next;
        if (offset >= sizeof(resp) - 1) break;
    }
    pthread_mutex_unlock(&data_lock);
    if (offset == 0) {
        snprintf(resp, sizeof(resp), "No surveys available.");
    }
    send(sockfd, resp, strlen(resp), 0);
}

// - list_vote_handler: 투표 목록 요청 처리
void list_vote_handler(int sockfd, char* msg) {
    char resp[BUFFER_SIZE] = {0};
    int offset = 0;
    pthread_mutex_lock(&data_lock);
    Vote* cur = vote_head;
    while (cur) {
        const char* status_str = (cur->status == STATUS_ACTIVE) ? "Active" : "Closed";
        offset += snprintf(resp + offset, sizeof(resp) - offset,
                           "[%s] ID: %s, Title: %s\n", status_str, cur->id, cur->title);
        cur = cur->next;
        if (offset >= sizeof(resp) - 1) break;
    }
    pthread_mutex_unlock(&data_lock);
    if (offset == 0) {
        snprintf(resp, sizeof(resp), "No votes available.");
    }
    send(sockfd, resp, strlen(resp), 0);
}

// - result_survey_handler: 설문 결과 요청 처리
void result_survey_handler(int sockfd, char* msg) 
{
    char resp[BUFFER_SIZE] = {0};
    char* saveptr;
    pthread_mutex_lock(&data_lock);
    strtok_r(msg, "|", &saveptr);
    char* id = strtok_r(NULL, "|", &saveptr);
    if (id == NULL) {
        pthread_mutex_unlock(&data_lock);
        send(sockfd, "[ERROR] Invalid format for RESULT_SURVEY", strlen("[ERROR] Invalid format for RESULT_SURVEY"), 0);
        return;
    }
    Survey* cur = survey_head;
    while (cur && strcmp(cur->id, id) != 0) {
        cur = cur->next;
    }
    if (!cur) {
        pthread_mutex_unlock(&data_lock);
        send(sockfd, "[ERROR] Survey not found", strlen("[ERROR] Survey not found"), 0);
        return;
    }
    int total = 0;
    for (int i = 0; i < cur->option_count; i++) {
        total += cur->votes[i];
    }
    int offset = 0;
    const char* status_str = (cur->status == STATUS_ACTIVE) ? "Active" : "Closed";
    offset += snprintf(resp + offset, sizeof(resp) - offset, "Question: %s [%s] (%d participants)\n", cur->question, status_str, cur->voter_count);
    for (int i = 0; i < cur->option_count; i++) {
        int pct = (total > 0) ? (cur->votes[i] * 100 / total) : 0;
        offset += snprintf(resp + offset, sizeof(resp) - offset,
                           "  %d. %s - %d votes (%d%%)\n",
                           i + 1, cur->options[i], cur->votes[i], pct);
        if (offset >= sizeof(resp) - 1) break;
    }
    pthread_mutex_unlock(&data_lock);
    send(sockfd, resp, strlen(resp), 0);
}

// - result_vote_handler: 투표 결과 요청 처리
void result_vote_handler(int sockfd, char* msg) {
    char resp[BUFFER_SIZE] = {0};
    char* saveptr;
    pthread_mutex_lock(&data_lock);
    strtok_r(msg, "|", &saveptr);
    char* id = strtok_r(NULL, "|", &saveptr);
    if (id == NULL) {
        pthread_mutex_unlock(&data_lock);
        send(sockfd, "[ERROR] Invalid format for RESULT_VOTE", strlen("[ERROR] Invalid format for RESULT_VOTE"), 0);
        return;
    }
    Vote* cur = vote_head;
    while (cur && strcmp(cur->id, id) != 0) {
        cur = cur->next;
    }
    if (!cur) {
        pthread_mutex_unlock(&data_lock);
        send(sockfd, "[ERROR] Vote not found", strlen("[ERROR] Vote not found"), 0);
        return;
    }
    int total = 0;
    for (int i = 0; i < cur->option_count; i++) {
        total += cur->votes[i];
    }
    int offset = 0;
    const char* status_str = (cur->status == STATUS_ACTIVE) ? "Active" : "Closed";
    offset += snprintf(resp + offset, sizeof(resp) - offset, "Title: %s [%s] (%d participants)\n", cur->title, status_str, cur->voter_count);
    for (int i = 0; i < cur->option_count; i++) {
        int pct = (total > 0) ? (cur->votes[i] * 100 / total) : 0;
        offset += snprintf(resp + offset, sizeof(resp) - offset,
                           "  %d. %s - %d votes (%d%%)\n",
                           i + 1, cur->options[i], cur->votes[i], pct);
        if (offset >= sizeof(resp) - 1) break;
    }
    pthread_mutex_unlock(&data_lock);
    send(sockfd, resp, strlen(resp), 0);
}