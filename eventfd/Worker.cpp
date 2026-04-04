#include "Worker.h"
#include <sys/select.h>
#include <sys/eventfd.h>
#include <unistd.h>
#include <print>

Worker::SelectTermination::SelectTermination() : efd(eventfd(0, 0)) {std::println("constructor efd: {}",efd);}

Worker::SelectTermination::~SelectTermination()
{
    std::println("[SelectTermination] destructor");
    if (efd > 0)
    {
        close(efd);
    }
}

void Worker::SelectTermination::notify()
{
    std::println("before notify");
    if (efd > 0)
    {
        uint64_t val = 1;
        auto res = write(efd, &val, sizeof(val));
        std::println("write {}",res);
    }
}

void Worker::work()
{
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(term_.efd, &rfds);

    std::println("before select");

    auto res = select(term_.efd+1, &rfds, NULL, NULL, NULL);

    std::println("after select res {}", res);

    if (FD_ISSET(term_.efd, &rfds))
    {
        std::println("event catched in select");
        uint64_t val;
        read(term_.efd, &val, sizeof(val)); // clear the event
    }
}

Worker::~Worker()
{
    std::println("[Worker] destructor");
    if (thr.joinable())
    {
        thr.join();
    }
}

void Worker::run()
{
    // TODO: make sure that joinable will be false if thread has been finished,
    // but user did not called join()
    if (thr.joinable())
    {
        /* code */
    }
    else
    {
        thr = std::thread(&Worker::work, this);
    }
}

void Worker::notify()
{
    term_.notify();
}
