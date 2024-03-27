//
// Created by czr on 24-3-17.
//


#include "Hook.h"
#include "IOSchedule.h"
#include "Log.h"
#include <cstring>
#include <netinet/in.h>
#include <arpa/inet.h>


void test_sleep() {
    Server::IOSchedule::ptr ioSchedule(new Server::IOSchedule(1));
    ioSchedule->post([]() {
        sleep(2);
        LOGI(LOG_ROOT()) << "test_sleep2...";
    });
    ioSchedule->post([]() {
        sleep(4);
        LOGI(LOG_ROOT()) << "test_sleep3...";
    });
    LOGI(LOG_ROOT()) << "test_sleep out...";
}

void test_socket() {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr{};
    memset(&addr, 0, sizeof(sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    //153.3.238.102　请求服务端的地址
    inet_pton(AF_INET, "153.3.238.102", &addr.sin_addr.s_addr);
    int rt = connect(sockfd, (const sockaddr *) &addr, sizeof(addr));
    LOGI(LOG_ROOT()) << "connect ret = " << rt << " errno=" << errno;
    if (rt) return;
    //发送Ｇｅｔ请求
    const char *cmd = "GET / HTTP/1.0\r\n\r\n";
    ssize_t ret = send(sockfd, cmd, strlen(cmd), 0);
    LOGI(LOG_ROOT()) << "send ret = " << ret << " errno=" << errno;

    //recv
    std::string buffer;
    buffer.resize(4096);
    ssize_t ret2 = recv(sockfd, &buffer[0], buffer.size(), 0);
    LOGI(LOG_ROOT()) << "recv ret = " << ret2 << " errno=" << errno;
    LOGI(LOG_ROOT()) << "response:\n" << buffer;
}

int main() {
    Server::IOSchedule ioSchedule;
    ioSchedule.post(test_socket);
}