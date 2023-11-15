/*
 * Copyright(c) 2023 All rights reserved by Heekuck Oh.
 * 이 프로그램은 한양대학교 ERICA 컴퓨터학부 학생을 위한 교육용으로 제작되었다.
 * 한양대학교 ERICA 학생이 아닌 이는 프로그램을 수정하거나 배포할 수 없다.
 * 프로그램을 수정할 경우 날짜, 학과, 학번, 이름, 수정 내용을 기록한다.
 * 2021083809 컴퓨터학부 유형진
 * #1 3/26, 컴퓨터학부, 2021083809, 유형진, 파일 리다이렉션 구현을 위한 배열 fd[] 생성한다. 
 * #2 3/26, 컴퓨터학부, 2021083809, 유형진, '<', '>' , '|', 기호가 아닌 인자만 저장한다.
 * #3 3/27, 컴퓨터학부, 2021083809, 유형진, '<' 파일 리다이렉션을 수행하는 코드이다.
 * #4 3/27, 컴퓨터학부, 2021083809, 유형진, '>' 파일 리다이렉션을 수행하는 코드이다.
 * #5 3/28, 컴퓨터학부, 2021083809, 유형진, 파이프를 위한 배열 fd_p을 생성한다.
 * #6 3/28, 컴퓨터학부, 2021083809, 유형진, fork()를 하기 위해 프로세서 ID pid_p를 생성한다. 
 * #7 3/29, 컴퓨터학부, 2021083809, 유형진, '|' 파이프를 수행하는 코드이다.
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_LINE 80             /* 명령어의 최대 길이 */

/*
 * cmdexec - 명령어를 파싱해서 실행한다.
 * 스페이스와 탭을 공백문자로 간주하고, 연속된 공백문자는 하나의 공백문자로 축소한다. 
 * 작은 따옴표나 큰 따옴표로 이루어진 문자열을 하나의 인자로 처리한다.
 * 기호 '<' 또는 '>'를 사용하여 표준 입출력을 파일로 바꾸거나,
 * 기호 '|'를 사용하여 파이프 명령을 실행하는 것도 여기에서 처리한다.
 */
static void cmdexec(char *cmd)
{
    char *argv[MAX_LINE/2+1];   /* 명령어 인자를 저장하기 위한 배열 */
    int argc = 0;               /* 인자의 개수 */
    char *p, *q;                /* 명령어를 파싱하기 위한 변수 */
    int fd[2];                  // #1
    int fd_p[2];                // #5
    pid_t pid_p;                // #6
    /*
     * 명령어 앞부분 공백문자를 제거하고 인자를 하나씩 꺼내서 argv에 차례로 저장한다.
     * 작은 따옴표나 큰 따옴표로 이루어진 문자열을 하나의 인자로 처리한다.
     */
    p = cmd; p += strspn(p, " \t");
    do {
        /*
         * 공백문자, 큰 따옴표, 작은 따옴표가 있는지 검사한다.
         */ 
        q = strpbrk(p, " \t\'\"");
        /*
         * 공백문자가 있거나 아무 것도 없으면 공백문자까지 또는 전체를 하나의 인자로 처리한다.
         */
        
        if (q == NULL || *q == ' ' || *q == '\t') {  
            q = strsep(&p, " \t");
            if (*q) {
                if(*q != '<' && *q != '>' && *q != '|')   // #2 이 코드를 통해서 명령어를 담는 argv[]배열에 리다이렉션 기호인 '<', '>' 와 파이프 기호인 '|' 가 들어가지 않게 한다.
                argv[argc++] = q;
            }
        }
        
        /*
         * 작은 따옴표가 있으면 그 위치까지 하나의 인자로 처리하고, 
         * 작은 따옴표 위치에서 두 번째 작은 따옴표 위치까지 다음 인자로 처리한다.
         * 두 번째 작은 따옴표가 없으면 나머지 전체를 인자로 처리한다.
         */
        else if (*q == '\'') {
            q = strsep(&p, "\'");
            if (*q) argv[argc++] = q;
            q = strsep(&p, "\'");
            if (*q) argv[argc++] = q;
        }
        /*
         * 큰 따옴표가 있으면 그 위치까지 하나의 인자로 처리하고, 
         * 큰 따옴표 위치에서 두 번째 큰 따옴표 위치까지 다음 인자로 처리한다.
         * 두 번째 큰 따옴표가 없으면 나머지 전체를 인자로 처리한다.
         */
        else {
            q = strsep(&p, "\"");
            if (*q) argv[argc++] = q;
            q = strsep(&p, "\"");
            if (*q) argv[argc++] = q;
        } 
        /* #3
         * q가 '<'기호를 만났을 때, 실행하도록 한다.
         * strsep()함수를 이용하여 q가 분리된 문자열를 가리키게 한다.
         * open()함수는 q 변수에 저장된 파일 이름을 이용하여 해당 파일을 읽기 전용 모드로 엽니다. 이 때 fd[0]에는 해당 파일에 대한 파일 디스크립터가 저장됩니다.
         * open()함수가 실패할 경우를 대비하여 실패할 경우 오류메세지를 출력하고 프로그램을 종료한다.
         * dup2()함수로 fd[0]에 저장된 파일 디스크립터를 표준 입력과 연결하여 파일에서부터 입력을 받을 수 있게 한다.
         * close()로 자원관리를 위해 fd[0]을 닫아준다.

         */

        if (*q == '<') {     
            q = strsep(&p, " ");          
            fd[0] = open(q,O_RDONLY);
            if (fd[0] < 0) {
                perror("perror tsh.c");
                exit(EXIT_FAILURE);
            }
            dup2(fd[0], STDIN_FILENO);
            close(fd[0]);
        }
        /* #4
         * q가 '>'기호를 만났을 때, 실행하도록 한다.
         * strsep()함수를 이용하여 q가 분리된 문자열을 가리키게 한다.
         * open()함수는 q 값을 파일이름으로 간주하고, 파일이 없다면 읽기와 쓰기가 가능한 파일을 생성한다. 또한 모든 사용자가 읽고 쓰기 권한을 가지도록 한다.
         * open()함수가 실패할 경우를 대비하여 실패할 경우 오류메세지를 출력하고 프로그램을 종료한다.
         * dup2()함수로 fd[1]에 저장된 파일 디스크립터를 표준 출력과 연결하여 파일로 출력을 할 수 있게 한다.
         * close()로 자원관리를 위해 fd[1]를 닫아준다.
         *          */

        else if (*q == '>') {
            q = strsep(&p, " "); 
            fd[1] = open(q,O_RDWR | O_CREAT, 0666);
            if (fd[1] < 0) {
                perror("perror delme");
                exit(EXIT_FAILURE);
            }
            dup2(fd[1], STDOUT_FILENO);
            close(fd[1]);          
        }
        
        /* #7
         * q가 '|'기호를 만났을 때, 실행하도록 한다.
         * pipe()함수를 호출하여 만약 문제가 생겼을 경우 오류메세지를 출력한 후 프로그램을 종료한다.
         * fork()함수를 호출하여 자식 프로세스를 생성한다. 만약 문제가 생겼을 경우 오류메세지를 출력한 후 프로그램을 종료한다.
         * 자식 프로세서 >>
         * 만약 pip_p == 0, 즉 자식 프로세스일 경우 dup2()함수를 통해 명령어의 표준 출력이 파이프로 출력되어 부모 프로세서에서 파이프의 입력을 받게한다.
         * argv[]배열의 끝에 NULL 값을 넣어 배열의 끝을 알 수 있게 한다.
         * execvp()함수로 argv[]에 저장된 명령어를 실행하도록 한다.
         * 자원관리를 위해 close()를 사용한다.
         * 
         * 부모 프로세서 >>
         * dup2()함수를 통해 명령어의 표준 입력을 파이프로 받을 수 있게 한다.
         * 재귀를 통해 남은 명령어를 실행하도록 한다. 
         * 자원관리를 위해 close()를 사용한다.
         */
        // #7
        if (*q == '|') {
            if (pipe(fd_p) == -1) {
                printf("pipe error");
                exit(EXIT_FAILURE);
            }

            if ((pid_p = fork()) == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
            }
        
            if (pid_p == 0) {
                close(fd_p[0]);
                dup2(fd_p[1],STDOUT_FILENO);
                close(fd_p[1]); 
                argv[argc] = NULL;
                execvp(argv[0],argv);                
            }
            else {  
                close(fd_p[1]);
                dup2(fd_p[0], STDIN_FILENO);
                close(fd_p[0]); 
                cmdexec(p);                              
            }   
        }  
    } while (p);
    
    argv[argc] = NULL;
    /*
     * argv에 저장된 명령어를 실행한다.
     */
    if (argc > 0)
        execvp(argv[0], argv);  
   
}

/*
 * 기능이 간단한 유닉스 셸인 tsh (tiny shell)의 메인 함수이다.
 * tsh은 프로세스 생성과 파이프를 통한 프로세스간 통신을 학습하기 위한 것으로
 * 백그라운드 실행, 파이프 명령, 표준 입출력 리다이렉션 일부만 지원한다.
 */
int main(void)
{
    char cmd[MAX_LINE+1];       /* 명령어를 저장하기 위한 버퍼 */
    int len;                    /* 입력된 명령어의 길이 */
    pid_t pid;                  /* 자식 프로세스 아이디 */
    int background;             /* 백그라운드 실행 유무 */
    
    /*
     * 종료 명령인 "exit"이 입력될 때까지 루프를 무한 반복한다.
     */
    while (true) {
        /*
         * 좀비 (자식)프로세스가 있으면 제거한다.
         */
        pid = waitpid(-1, NULL, WNOHANG);
        if (pid > 0)
            printf("[%d] + done\n", pid);
        /*
         * 셸 프롬프트를 출력한다. 지연 출력을 방지하기 위해 출력버퍼를 강제로 비운다.
         */
        printf("tsh> "); fflush(stdout);
        /*
         * 표준 입력장치로부터 최대 MAX_LINE까지 명령어를 입력 받는다.
         * 입력된 명령어 끝에 있는 새줄문자를 널문자로 바꿔 C 문자열로 만든다.
         * 입력된 값이 없으면 새 명령어를 받기 위해 루프의 처음으로 간다.
         */
        len = read(STDIN_FILENO, cmd, MAX_LINE);
        if (len == -1) {
            perror("read");
            exit(EXIT_FAILURE);
        }
        cmd[--len] = '\0';
        if (len == 0)
            continue;
        /*
         * 종료 명령이면 루프를 빠져나간다.
         */
        if(!strcasecmp(cmd, "exit"))
            break;
        /*
         * 백그라운드 명령인지 확인하고, '&' 기호를 삭제한다.
         */
        char *p = strchr(cmd, '&');
        if (p != NULL) {
            background = 1;
            *p = '\0';
        }
        else
            background = 0;
        /*
         * 자식 프로세스를 생성하여 입력된 명령어를 실행하게 한다.
         */
        if ((pid = fork()) == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        }
        /*
         * 자식 프로세스는 명령어를 실행하고 종료한다.
         */
        else if (pid == 0) {
            cmdexec(cmd);
            exit(EXIT_SUCCESS);
        }
        /*
         * 포그라운드 실행이면 부모 프로세스는 자식이 끝날 때까지 기다린다.
         * 백그라운드 실행이면 기다리지 않고 다음 명령어를 입력받기 위해 루프의 처음으로 간다.
         */
        else if (!background)
            waitpid(pid, NULL, 0);
    }
    return 0;
}