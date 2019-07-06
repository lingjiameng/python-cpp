/*
trans img from cpp to python
输入命令编译
g++ `pkg-config opencv --cflags` cppserver.cpp  -o tmp `pkg-config opencv --libs`

*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <iostream>
#include <iterator>
#include <stdio.h>
#include <limits.h>

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/opencv.hpp>


//vetor转换为string，并用逗号分隔
std::string vec2str(const std::vector<uchar> &src)
{
    std::ostringstream oss;
    std::copy(src.begin(), src.end() - 1, std::ostream_iterator<int>(oss, ","));
    oss << int(src.back());
    return oss.str();
}

//向指定tcp连接发送图片数据包（全部为string格式）
//发送流程：发送图片长度->接收图片长度ACK->分包发送图片
int send(int client_sockfd,const cv::Mat &src){
    std::vector<uchar> b_img; //二进制格式jpg
    std::string s_img; //二进制jpg-> bytes ->uint8 ->用逗号拼接为的string
    char buf[BUFSIZ]; //buffer
    std::string s_img_len;//字符串格式的长度

    //分段发送数据
    int len = 0; //接收数据的长度
    int sended = 0; //已经发送的数据的长度
    int s_len = 0; //发送数据的长度
    int to_s_len = 0; //要发送的数据的长度

    //图片压缩为二进制数，并用uint8数组形式存储
    cv::imencode(".jpg", src, b_img);
    //将数组以逗号分隔拼接为string
    s_img = vec2str(b_img);
    //图片大小存入buffer进行传输
    s_img_len = std::to_string(s_img.size());


    //发送图片大小并等待确认
    s_len = send(client_sockfd, s_img_len.c_str(), s_img_len.size(), 0);
    if (s_len <= 0) return 1;

    //对图片大小的确认
    len = recv(client_sockfd, buf, BUFSIZ, 0);
    if (len <= 0) return 1;
    //接收到命令
    buf[len] = '\0';
    printf("[Image size: %s][ %s ] \n", s_img_len.c_str(), buf);

    for (; sended < s_img.size(); sended += s_len)
    {
        //对数据进行分包
        to_s_len = std::min(BUFSIZ, int(s_img.size() - sended));
        //发送分完相应的包
        s_len = send(client_sockfd, s_img.c_str() + sended, to_s_len, 0);
        if (s_len < 0)return 1;
    }
    printf("[trans image finished][len: %i][sended: %i/%lu]\n", s_len, sended, s_img.size());
    return 0;
}

int main(int argc, char *argv[])
{
    int server_sockfd; //服务器端套接字
    int client_sockfd; //客户端套接字
    int len;
    struct sockaddr_in my_addr;     //服务器网络地址结构体
    struct sockaddr_in remote_addr; //客户端网络地址结构体
    unsigned int sin_size;
    char buf[BUFSIZ];                     //数据传送的缓冲区
    memset(&my_addr, 0, sizeof(my_addr)); //数据初始化--清零
    my_addr.sin_family = AF_INET;         //设置为IP通信
    my_addr.sin_addr.s_addr = INADDR_ANY; //服务器IP地址--允许连接到所有本地地址上
    my_addr.sin_port = htons(13456);      //服务器端口号

    /*创建服务器端套接字--IPv4协议，面向连接通信，TCP协议*/
    if ((server_sockfd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket error");
        return 1;
    }

    /*将套接字绑定到服务器的网络地址上*/
    if (bind(server_sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) < 0)
    {
        perror("bind error");
        return 1;
    }

    /* 监听连接请求--监听队列长度为5 */
    if (listen(server_sockfd, 5) < 0)
    {
        perror("listen error");
        return 1;
    };

    sin_size = sizeof(struct sockaddr_in);

    ///////////////////////////////
    // 循环开始

    /*等待客户端连接请求到达*/
    while ((client_sockfd = accept(server_sockfd, (struct sockaddr *)&remote_addr, &sin_size)) > 0)
    {

        printf("Accept client %s\n", inet_ntoa(remote_addr.sin_addr));

        /*接收客户端的数据并将其发送给客户端--recv返回接收到的字节数，send返回发送的字节数*/

        while ((len = recv(client_sockfd, buf, BUFSIZ, 0)) > 0)
        {
            //接收到命令
            buf[len] = '\0';
            printf("Cmd from client:[ %s ]\n", buf);
            
            //根据指令读取图片
            cv::Mat img = cv::imread("car.jpg");
            cv::Size size = cv::Size(416, 416);
            cv::resize(img, img, size);
            //发送图片
            send(client_sockfd,img);
        }
        printf("closed\n");
        /*关闭套接字*/
        shutdown(client_sockfd, SHUT_RDWR);
    }
    shutdown(server_sockfd, SHUT_RDWR);
    perror("accept error");
    return 1;

    return 0;
}