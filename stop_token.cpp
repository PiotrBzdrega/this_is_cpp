#include <cstdio>
#include <thread>
#include <chrono>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>

using namespace std::chrono_literals;

const int PORT = 8080;

void server(std::stop_token token)
{
    while (!token.stop_requested())
    {
        // 1. Create socket
        int server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd < 0) 
        {
            std::printf("Failed to create socket\n");
            return;
        }

        // 2. Bind socket to port
        sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY;
        server_addr.sin_port = htons(PORT);

        if (bind(server_fd, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) 
        {
            std::printf("Failed to bind\n");
            return;
        }

        // 3. Listen for connections
        if (listen(server_fd, 1) < 0) {
            std::printf("Failed to listen\n");
            return ;
        }

        std::printf("Server listening on port %d\n",PORT);

        std::stop_callback cb(token,[&]
            {
                shutdown(server_fd,SHUT_RDWR);
                close(server_fd);
                std::printf("shutdown\n");
            });

        // 4. Accept a connection
        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) 
        {
            std::printf("Failed to accept %s\n",strerror(errno));
            return;
        }

        read()
    }
    


}

void worker(std::stop_token token, int &irq)
{
    auto this_id = std::this_thread::get_id();
    std::printf("[%s] thread id:%lld\n",__FUNCTION__,this_id);

    std::stop_callback cb(token,[&]
        {
            auto callback_id = std::this_thread::get_id();
            std::printf("[%s] callback thread id:%lld\n",__FUNCTION__,this_id);
        });

    while (true)
    {
        std::this_thread::sleep_for(1s);
        std::printf("loop\n");
    }
    std::printf("finished \n");
}



int main()
{
    auto worker_thread = std::jthread(server);
    
    std::this_thread::sleep_for(5s);

    worker_thread.join();
    // worker_thread.request_stop();
}