//std::launch::async each task runs in its own thread
auto task = std::async(std::launch::async, call_process);

task.wait();
