#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h> // 包含 getaddrinfo/freeaddrinfo
#include <arpa/inet.h>

#define PORT "8080"
#define BUFFER_SIZE 1024

void error(const char *msg) {
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[]) {
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char buffer[BUFFER_SIZE];
    char ipstr[INET6_ADDRSTRLEN];

    if (argc != 2) {
        fprintf(stderr,"用法: %s <服务器IPv4/IPv6地址>\n", argv[0]);
        fprintf(stderr,"示例: %s 127.0.0.1  (使用 IPv4)\n", argv[0]);
        fprintf(stderr,"示例: %s ::1        (使用 IPv6)\n", argv[0]);
        exit(1);
    }

    const char *server_ip = argv[1];

    printf("--- IPv4/IPv6 客户端启动中 ---\n");

    // 1. 使用 getaddrinfo 获取服务器地址信息
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;  // 允许 IPv4 或 IPv6
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(server_ip, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo 错误: %s\n", gai_strerror(rv));
        return 1;
    }

    // 遍历结果链表，直到成功连接
    for (p = servinfo; p != NULL; p = p->ai_next) {
        printf("\n==========================================\n");
        
        // 关键演示点：获取协议族信息
        if (p->ai_family == AF_INET) {
            printf("[GETADDRINFO] 尝试连接协议族: **IPv4 (AF_INET)**\n");
            struct sockaddr_in *s = (struct sockaddr_in *)p->ai_addr;
            inet_ntop(AF_INET, &(s->sin_addr), ipstr, sizeof(ipstr));
            printf("[TARGET_IP] 目标 IPv4 地址: %s\n", ipstr);
        } else if (p->ai_family == AF_INET6) {
            printf("[GETADDRINFO] 尝试连接协议族: **IPv6 (AF_INET6)**\n");
            struct sockaddr_in6 *s = (struct sockaddr_in6 *)p->ai_addr;
            inet_ntop(AF_INET6, &(s->sin6_addr), ipstr, sizeof(ipstr));
            printf("[TARGET_IP] 目标 IPv6 地址: %s\n", ipstr);
        } else {
            printf("[GETADDRINFO] 尝试连接协议族: 未知 (%d)\n", p->ai_family);
            continue;
        }

        // 2. 创建套接字
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("[SOCKET] 客户端创建套接字失败");
            continue;
        }
        printf("[SOCKET] 成功创建套接字描述符: %d\n", sockfd);

        // 3. 连接服务器
        printf("[CONNECT] 正在连接...\n");
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("[CONNECT] 连接失败");
            continue;
        }
        
        printf("[SUCCESS] **连接成功！**\n");
        break; // 连接成功，跳出循环
    }

    if (p == NULL) {
        fprintf(stderr, "[FATAL] 客户端连接服务器失败\n");
        return 2;
    }

    freeaddrinfo(servinfo); // 释放 getaddrinfo 结果

    // 4. 简单的读写交互
    const char *msg = "Hello from the C client!";
    printf("[SEND] 发送数据: \"%s\"\n", msg);
    if (send(sockfd, msg, strlen(msg), 0) == -1) {
        error("ERROR writing to socket");
    }

    memset(buffer, 0, BUFFER_SIZE);
    ssize_t n = recv(sockfd, buffer, BUFFER_SIZE - 1, 0);
    if (n > 0) {
        printf("[RECV] 收到服务器响应: \"%s\"\n", buffer);
    } else if (n == 0) {
        printf("[RECV] 服务器关闭了连接。\n");
    } else {
        error("ERROR reading from socket");
    }

    // 5. 关闭套接字
    close(sockfd);
    printf("[CLOSE] 套接字已关闭。\n");
    printf("==========================================\n");

    return 0;
}
