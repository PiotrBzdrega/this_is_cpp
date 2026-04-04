/**
 * TODO: create:
 * parent - dummy - wrapper - demon app
 * finish dummy, then wrapper will be linked to (1)
 * wait in parent for return result from wrapper using unix socket or shm and sem
 * TODO: why dummy process is linked to parent even if died
 * TODO: understand double fork https://thelinuxjedi.blogspot.com/2014/02/why-use-double-fork-to-daemonize.html
 */

#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <iostream>
#include <thread>

int main() {
    // int pipefd[2];
    // if (pipe(pipefd) == -1) {
    //     perror("pipe");
    //     return 1;
    // }

    pid_t wrapper_pid = fork();

    if (wrapper_pid == -1) {
        perror("fork");
        return 1;
    }

    if (wrapper_pid == 0) {
        // =========================
        // WRAPPER PROCESS
        // =========================

        // close(pipefd[0]); // close read end

        // Detach (optional but typical)
        if (setsid() == -1) {
            perror("setsid");
        }

        // Fork again to run the real app
        pid_t child_pid = fork();

        if (child_pid == -1) {
            int err = -1;
            // write(pipefd[1], &err, sizeof(err));
            _exit(1);
        }

        if (child_pid == 0) {
            // =========================
            // TARGET APP (unmodified)
            // =========================
            std::cout << "Effective child application\n";

            // Important: keep pipefd[1] open!

            execl("/bin/sh", "sh", "-c", "sleep 30; exit 300", nullptr);

            // exec failed
            _exit(127);
        }

        // =========================
        // WRAPPER waits for target
        // =========================
        std::cout << "Wait for result from child : " << child_pid << "\n";

        // int status;
        // if (waitpid(child_pid, &status, 0) == -1) {
        //     int err = -2;
        //     // write(pipefd[1], &err, sizeof(err));
        //     _exit(1);
        // }

        // int result = -3;

        // if (WIFEXITED(status)) {
        //     result = WEXITSTATUS(status); // normal exit
        // } else if (WIFSIGNALED(status)) {
        //     result = 128 + WTERMSIG(status); // signal-based exit
        // }

        // Send result to parent
        // if (write(pipefd[1], &result, sizeof(result)) == -1) {
        //     perror("write");
        // }

        // close(pipefd[1]);
        _exit(0);
    }

    // =========================
    // PARENT PROCESS
    // =========================
    std::cout << "Wait for result from wrapper : " << wrapper_pid << "\n";

    std::this_thread::sleep_for(std::chrono::seconds(60));

    // close(pipefd[1]); // close write end

    // int result;
    // ssize_t n = read(pipefd[0], &result, sizeof(result));

    // if (n == sizeof(result)) {
    //     std::cout << "Received result: " << result << "\n";
    // } else {
    //     std::cerr << "Failed to read result\n";
    // }

    // close(pipefd[0]);

    // // Reap wrapper (avoid zombie)
    // waitpid(wrapper_pid, nullptr, 0);

    return 0;
}