#include <string>
#include <cstdio>
#include <semaphore>
#include <thread>
#include <chrono>
#include <iostream>

using namespace std::chrono;

struct sharedData
{
    std::binary_semaphore master;   /* master write */
    std::binary_semaphore slave;    /* slave  write */
    std::string request;
    std::string response;

    sharedData() :master(0),slave(0){};
};

void request(std::string& str)
{
    std::printf("[%s,%d]\n",__FUNCTION__,__LINE__);
}

void oslp(sharedData& shD)
{
    std::printf("[%s,%d] Press 'p' then enter to start ping pong\n",__FUNCTION__,__LINE__);
    int requestCounter=0;
    char ch;
    while (std::cin.get(ch))
    {
        if (ch =='p')
        {
            std::printf("Character p\n");
            break;
        }
    }

    while (true)
    {
        shD.request = std::to_string(requestCounter);
        std::printf("[%s,%d] master sent msg: %s \n",__FUNCTION__,__LINE__,shD.request.data());
        requestCounter++;
        shD.master.release();

        shD.slave.acquire();
        std::printf("[%s,%d] master received msg: %s\n",__FUNCTION__,__LINE__,shD.response.data());
        

        std::this_thread::sleep_for( seconds(3));
    }
}

void response()
{
    std::printf("[%s,%d]\n",__FUNCTION__,__LINE__);
}

void slitap(sharedData& shD)
{
    while (true)
    {
        if(shD.master.try_acquire_for(seconds(1)))
        {
            shD.response = shD.request;
            shD.slave.release();
        }
        else
        {
            std::printf("[%s,%d] could not acquire oslpToSlitap sem \n",__FUNCTION__,__LINE__);
        }
        
        std::this_thread::sleep_for( seconds(2));
    }
}

int main()
{
    std::string msg;
    std::binary_semaphore oslpToSlitap(0);
    std::binary_semaphore slitapToOslp(0);
    sharedData sharedD;

    std::jthread t1(oslp,std::ref(sharedD));
    std::jthread t2(slitap,std::ref(sharedD));

    return 0;
}