네트워크 기반 설문조사 및 투표 시스템 (C언어)
본 문서는 C언어 및 TCP/IP 소켓 API를 기반으로 구현된 콘솔 환경의 설문조사 및 투표 시스템에 대한 기술 명세서입니다. 본 시스템의 핵심 아키텍처는 다중 클라이언트의 동시 접속 및 상호작용을 지원하기 위한 멀티스레딩이며, 데이터 관리의 안정성과 사용자 중심의 기능 제공을 핵심 목표로 설정하였습니다.

📋 핵심 기능
실시간 설문 및 투표 생성과 참여 기능: 사용자는 시스템을 통해 설문 또는 투표 항목을 동적으로 생성할 수 있으며, 타 사용자는 해당 항목에 실시간으로 참여하여 자신의 의견을 기록할 수 있습니다.

멀티스레딩을 통한 다중 사용자 동시 접속 지원: POSIX 스레드(pthread) 라이브러리를 기반으로 멀티스레드 서버를 구축하여, 다수의 클라이언트가 동시에 접속하여 요청을 전송하더라도 안정적인 병렬 처리가 가능합니다.

파일 시스템 기반의 데이터 영속성 보장: 생성된 모든 설문 및 투표 항목의 메타데이터, 응답 결과, 참여자 정보는 서버의 로컬 파일 시스템 내 텍스트 파일에 영구적으로 기록됩니다. 이를 통해 서버 프로세스가 재시작되더라도 모든 데이터가 일관성 있게 복원됩니다.

제목 기반의 사용자 친화적 ID 자동 생성: survey-001과 같은 기계적 식별자 대신, 사용자가 입력한 제목을 기반으로 인간이 인지하기 용이한(Human-readable) ID를 동적으로 생성합니다.

항목별 상태 관리 기능: 각 설문 및 투표 항목은 '진행중(Active)'과 '종료(Closed)'라는 명시적인 상태를 가지며, 관리자는 특정 항목의 참여를 비활성화하여 마감 처리할 수 있습니다.

사용자 이름 기반의 중복 참여 방지 메커니즘: 각 클라이언트는 고유한 사용자 이름으로 식별되며, 서버는 각 항목에 대한 참여자 명단을 기록합니다. 이를 통해 동일 사용자의 중복 응답을 원천적으로 차단하여 데이터의 신뢰도를 확보합니다.

🔧 기술적 특징 상세
본 프로젝트는 단순한 기능 구현을 넘어 시스템의 안정성과 확장성을 확보하기 위해, 다음과 같은 컴퓨터 공학적 원리들을 적용하여 설계되었습니다.

1. 멀티스레딩과 동기화 (Concurrency & Synchronization)
독립적인 요청 처리 모델: Main 스레드는 accept()를 통해 블로킹 상태로 클라이언트의 연결을 대기하며, 연결 수립 시 즉시 pthread_create()를 호출하여 해당 클라이언트와의 통신을 위한 Worker 스레드를 생성합니다. 이 "One-Thread-Per-Client" 모델은 각 클라이언트의 요청이 독립된 실행 흐름(Thread of execution) 내에서 처리되도록 보장하여, 특정 요청의 지연이 다른 클라이언트의 서비스에 영향을 미치지 않도록 합니다.

경쟁 상태(Race Condition) 방지: 설문 및 투표 데이터가 저장된 전역 연결 리스트는 모든 스레드가 접근하는 **공유 자원(Shared Resource)**입니다. 다수의 스레드가 이 공유 자원을 동시에 수정할 경우 데이터의 일관성이 파괴될 수 있으므로, pthread_mutex_t를 사용한 상호 배제(Mutual Exclusion) 메커니즘을 구현하였습니다. 데이터 구조에 접근하는 모든 코드 영역을 임계 구역(Critical Section)으로 설정하고 mutex 잠금(Lock)으로 보호함으로써, 한 번에 하나의 스레드만이 데이터 수정을 수행하도록 보장하여 원자성(Atomicity)을 확보합니다.

스레드 안전성(Thread-Safety) 확보: C 표준 라이브러리의 strtok 함수는 내부적으로 정적 버퍼를 사용하여 재진입이 불가능(Non-reentrant)하므로, 멀티스레드 환경에서 호출될 시 심각한 데이터 오염을 유발할 수 있습니다. 이러한 문제를 회피하기 위해, 상태 저장용 포인터를 명시적으로 전달하여 각 스레드가 독립적인 파싱 컨텍스트를 유지할 수 있도록 하는 스레드 안전 함수 strtok_r로 전면 대체하였습니다.

2. 데이터 영속성 모델 (Data Persistence Model)
파일 기반 저장소 아키텍처: 본 시스템은 별도의 데이터베이스 관리 시스템(DBMS)에 대한 의존성 없이, 표준 파일 입출력(I/O) API만을 사용하여 데이터의 영속성을 구현합니다. 각 설문/투표 인스턴스는 고유 ID를 파일명으로 하는 .txt 파일에 직렬화(serialize)되어 저장됩니다.

구조화된 파일 포맷 설계: 데이터의 파싱 및 복원 효율성을 위해 파일 내 데이터는 다음과 같은 명확한 구조를 따릅니다.

[Line 1]: Title or Question String
[Line 2]: Status Code (0: Active, 1: Closed)
[...]: Option_String:Vote_Count
[Delimiter]: ---VOTERS---
[...]: Username_List

즉시 동기화 전략: 사용자의 응답으로 인해 메모리 상의 데이터가 변경될 경우, 해당 변경 사항은 즉시 파일에 반영됩니다. 이는 파일 전체를 새로 덮어쓰는(Overwrite) 방식으로 구현되어, 메모리와 스토리지 간의 데이터 일관성을 유지하고 예기치 않은 서버 종료 시 발생할 수 있는 데이터 유실을 최소화합니다.

3. 사용자 정의 통신 프로토콜
클라이언트와 서버 간의 모든 어플리케이션 계층 통신은 | 문자를 구분자(Delimiter)로 사용하는 텍스트 기반 프로토콜로 표준화되었습니다.

요청 형식: COMMAND|PARAMETER_1|PARAMETER_2|...|USERNAME

이러한 프로토콜 기반의 설계는 향후 새로운 기능 추가 시, 신규 명령어와 파라미터 구조를 정의하는 것만으로 시스템을 용이하게 확장할 수 있는 유연성을 제공합니다.

🚀 빌드 및 실행 방법
사전 요구사항
gcc 컴파일러

make 빌드 자동화 유틸리티

pthread 라이브러리

1. 소스 코드 컴파일
프로젝트의 루트 디렉토리에서 다음의 make 명령어를 실행하면, Makefile에 정의된 규칙에 따라 src/server/와 src/client/ 디렉토리 내에 각각 실행 파일(server, client)이 생성됩니다.

make

2. 서버 실행
새로운 터미널 세션을 열고, 다음 명령어를 통해 서버 프로세스를 실행합니다.

./src/server/server

서버가 정상적으로 구동되면, >> Server listening on port 9000 메시지가 콘솔에 출력됩니다.

3. 클라이언트 실행
서버 실행과 별개의 터미널 세션에서, 다음 명령어를 통해 클라이언트 프로그램을 실행합니다.

./src/client/client

클라이언트 프로그램이 시작되면 사용자 이름 입력을 위한 프롬프트가 표시되며, 이름 입력 완료 후 메뉴 시스템을 통해 서버의 기능을 이용할 수 있습니다. 다중 접속 환경을 시뮬레이션하기 위해 복수의 클라이언트 인스턴스를 실행할 수 있습니다.

📂 디렉토리 구조
.
├── include
│   └── common.h          # 클라이언트-서버 공용 데이터 구조체 및 상수 정의
├── src
│   ├── client
│   │   └── client_main.c   # 클라이언트 프로그램의 주요 로직
│   └── server
│       └── server_main.c   # 서버 프로그램의 주요 로직
├── data                    # 설문 및 투표 데이터 파일 저장 디렉토리 (자동 생성)
│   ├── survey
│   └── vote
├── Makefile                # 빌드 자동화를 위한 Makefile
└── README.md

Network-based Survey and Polling System in C
This document provides the technical specifications for a console-based survey and polling system implemented in C, utilizing the TCP/IP socket API. The core architecture of this system is multithreading to support concurrent client connections and interactions, with primary objectives focused on ensuring data management stability and providing user-centric functionalities.

📋 Core Features
Real-time Survey and Poll Creation and Participation: Users can dynamically create survey or poll items through the system, and other users can participate in these items in real-time to record their opinions.

Multi-user Concurrency Support via Multithreading: A multithreaded server, built upon the POSIX Threads (pthread) library, is implemented to enable stable parallel processing of requests from numerous concurrently connected clients.

Data Persistence via File System: All metadata, response results, and participant information for every survey and poll item are permanently recorded in text files within the server's local file system. This ensures that all data is consistently restored upon a server restart.

User-Friendly, Title-based ID Generation: Instead of mechanical identifiers like survey-001, the system dynamically generates human-readable IDs based on user-input titles.

Item-specific Status Management: Each survey and poll item possesses an explicit state of 'Active' or 'Closed', allowing an administrator to deactivate participation for a specific item, effectively closing it.

Username-based Duplicate Participation Prevention: Each client is identified by a unique username, and the server maintains a list of participants for each item. This mechanism prevents the same user from submitting multiple responses, thereby ensuring data integrity.

🔧 Detailed Technical Features
This project was designed by applying the following computer science principles to ensure system stability and scalability, moving beyond simple functional implementation.

1. Concurrency and Synchronization
Independent Request Processing Model: The main thread waits for client connections in a blocking state via accept(). Upon establishing a connection, it immediately invokes pthread_create() to spawn a new worker thread dedicated to handling communication with that client. This "One-Thread-Per-Client" model guarantees that each client's request is processed within an independent thread of execution, preventing delays from one request from affecting the service for other clients.

Race Condition Prevention: The global linked list storing survey and poll data is a Shared Resource accessible by all threads. To prevent data corruption from simultaneous modifications by multiple threads, a mutual exclusion mechanism using pthread_mutex_t is implemented. All code segments that access the data structure are defined as a Critical Section and are protected by a mutex lock, which ensures atomicity by allowing only one thread to perform data modifications at a time.

Ensuring Thread-Safety: The strtok function from the C standard library is non-reentrant due to its use of an internal static buffer, which can cause severe data corruption when called in a multithreaded environment. To circumvent this issue, it has been entirely replaced with strtok_r, a thread-safe alternative that maintains each thread's parsing context independently by explicitly passing a state-saving pointer.

2. Data Persistence Model
File-based Storage Architecture: The system implements data persistence using only standard file I/O APIs, without dependency on an external Database Management System (DBMS). Each survey or poll instance is serialized and stored in a .txt file, named with its unique ID.

Structured File Format Design: For efficiency in data parsing and restoration, the data within each file adheres to the following clear structure:

[Line 1]: Title or Question String
[Line 2]: Status Code (0: Active, 1: Closed)
[...]: Option_String:Vote_Count
[Delimiter]: ---VOTERS---
[...]: Username_List

Immediate Synchronization Strategy: When data in memory is altered due to a user's response, the changes are immediately reflected in the corresponding file. This is implemented by overwriting the entire file, which maintains data consistency between memory and storage and minimizes data loss in the event of an unexpected server shutdown.

3. Custom Communication Protocol
All application-layer communication between the client and server is standardized through a text-based protocol using the | character as a delimiter.

Request Format: COMMAND|PARAMETER_1|PARAMETER_2|...|USERNAME

This protocol-based design provides the flexibility to easily extend the system with new functionalities by simply defining new commands and parameter structures.

🚀 Build and Execution
Prerequisites
gcc compiler

make build automation utility

pthread library

1. Compile Source Code
From the project's root directory, execute the make command. This will generate executable files (server, client) in the src/server/ and src/client/ directories, respectively, according to the rules defined in the Makefile.

make

2. Run the Server
Open a new terminal session and execute the following command to run the server process.

./src/server/server

Upon successful startup, the message >> Server listening on port 9000 will be displayed on the console.

3. Run the Client
In a separate terminal session, execute the following command to run the client program.

./src/client/client

When the client program starts, a prompt will appear for username input. After entering a name, you can interact with the system through the menu. Multiple client instances can be run to simulate a multi-user environment.

📂 Directory Structure
.
├── include
│   └── common.h          # Common data structures and constants for client-server
├── src
│   ├── client
│   │   └── client_main.c   # Main logic for the client program
│   └── server
│       └── server_main.c   # Main logic for the server program
├── data                    # Directory for storing survey and poll data files (auto-generated)
│   ├── survey
│   └── vote
├── Makefile                # Makefile for build automation
└── README.md
