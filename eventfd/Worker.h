#include <thread>

class Worker
{
private:
  struct SelectTermination
  {
    int efd; // interruption file descriptor to wakeup select
    SelectTermination();
    ~SelectTermination();
    void notify();
  } term_;
  std::thread thr;
  void work();
public:
  Worker(/* args */) = default;
  ~Worker();
  void run();
  void notify();
};