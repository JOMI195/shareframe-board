#include "util/Subprocess.hpp"
#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstring>
#include <poll.h>
#include <sys/wait.h>
#include <unistd.h>

SubprocessResult Subprocess::run(const std::vector<std::string>& args, int timeoutSecs)
{
    if (args.empty())
        return {-1, "", "empty command"};

    // Build null-terminated argv array
    std::vector<const char*> argv;
    argv.reserve(args.size() + 1);
    for (const auto& a : args)
        argv.push_back(a.c_str());
    argv.push_back(nullptr);

    int outPipe[2];
    int errPipe[2];
    if (pipe(outPipe) < 0 || pipe(errPipe) < 0)
        return {-1, "", std::string("pipe() failed: ") + strerror(errno)};

    pid_t pid = fork();
    if (pid < 0)
    {
        close(outPipe[0]); close(outPipe[1]);
        close(errPipe[0]); close(errPipe[1]);
        return {-1, "", std::string("fork() failed: ") + strerror(errno)};
    }

    if (pid == 0)
    {
        // Child
        close(outPipe[0]);
        close(errPipe[0]);
        if (dup2(outPipe[1], STDOUT_FILENO) < 0 || dup2(errPipe[1], STDERR_FILENO) < 0)
            _exit(126);
        close(outPipe[1]);
        close(errPipe[1]);

        execvp(argv[0], const_cast<char* const*>(argv.data()));
        _exit(127); // execvp failed
    }

    // Parent
    close(outPipe[1]);
    close(errPipe[1]);

    std::string stdOut, stdErr;
    char buf[4096];
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(timeoutSecs);

    int openFds = 2;
    while (openFds > 0)
    {
        auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
            deadline - std::chrono::steady_clock::now());
        if (remaining.count() <= 0)
        {
            kill(pid, SIGKILL);
            waitpid(pid, nullptr, 0);
            close(outPipe[0]);
            close(errPipe[0]);
            return {-1, stdOut, "timeout after " + std::to_string(timeoutSecs) + "s"};
        }

        pollfd fds[2];
        int nfds = 0;
        if (outPipe[0] >= 0) { fds[nfds++] = {outPipe[0], POLLIN, 0}; }
        if (errPipe[0] >= 0) { fds[nfds++] = {errPipe[0], POLLIN, 0}; }

        int ret = poll(fds, nfds, static_cast<int>(remaining.count()));
        if (ret < 0)
        {
            if (errno == EINTR) continue;
            break;
        }

        for (int i = 0; i < nfds; ++i)
        {
            if (!(fds[i].revents & (POLLIN | POLLHUP)))
                continue;

            ssize_t n = read(fds[i].fd, buf, sizeof(buf));
            if (n > 0)
            {
                if (fds[i].fd == outPipe[0])
                    stdOut.append(buf, n);
                else
                    stdErr.append(buf, n);
            }
            else
            {
                close(fds[i].fd);
                if (fds[i].fd == outPipe[0]) outPipe[0] = -1;
                else errPipe[0] = -1;
                --openFds;
            }
        }
    }

    if (outPipe[0] >= 0) close(outPipe[0]);
    if (errPipe[0] >= 0) close(errPipe[0]);

    int status = 0;
    waitpid(pid, &status, 0);
    int exitCode = WIFEXITED(status) ? WEXITSTATUS(status) : -1;

    return {exitCode, stdOut, stdErr};
}
