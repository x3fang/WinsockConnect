#ifndef LOG_H
#define LOG_H
#include <map>
#include <fstream>
#include <string>
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#include <direct.h>
#include <io.h>
#endif
#if __linux__
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#endif
#include <vector>
#include <ctime>
#include <atomic>
#include <sys/stat.h>
namespace logNameSpace
{
    // 通过stat结构体 获得文件大小，单位字节
    size_t getFileSize1(const char *fileName)
    {

        if (fileName == NULL)
        {
            return 0;
        }

        struct stat statbuf;

        ::stat(fileName, &statbuf);

        size_t filesize = statbuf.st_size;

        return filesize;
    }

    class Log;
    class funLog
    {
    public:
        funLog() = default;
        funLog(const std::string name, Log *log)
        {
            this->name = name;
            this->log = log;
        }
        void write(const std::string &msg);
        void write(std::vector<std::string> &msg);
        funLog &operator<<(const std::string &msg);
        funLog &operator<<(const int &msg);

    private:
        std::string name;
        Log *log;
    };
    class Log
    {
    public:
        Log() = default;
        Log(const std::string name, int logMaxSize = 1024 * 1024 * 128)
        {
            this->writeFlie = new std::atomic<bool>(false);
            this->logName = name;
            mustChangeFlie();
            
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
            createDirectory("log");
#endif
#if __linux__
            createDirectory("/"+logName+"-log");
#endif
            this->logMaxSize = logMaxSize;
        }
        ~Log()
        {
            if (!msg.empty())
            {
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
                std::ofstream logFilet("log\\" + logName + ".log", std::ios::out | std::ios::app);
#endif
#if __linux__
                std::ofstream logFilet("/" + logName + "-log/" + logName + ".log", std::ios::out | std::ios::app);
#endif
                logFilet << getTime() << " " << msg << std::endl;
                logFilet.close();
            }
        }
        Log &operator=(Log &&other) noexcept
        {
            this->msg = other.msg;
            this->logName = other.logName;
            this->logMaxSize = other.logMaxSize;
            this->writeFlie = other.writeFlie;
            return *this;
        }
        void mustChangeFlie()
        {
            std::string temp = logName;
            for (int i = 0; true; i++)
            {
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
                int flieSize = getFileSize1(("log\\" + temp + ".log").c_str());
#endif
#if __linux__
                int flieSize = getFileSize1(("/"+logName+"-log/" + temp + ".log").c_str());
#endif
                if (flieSize >= logMaxSize)
                {
                    if (i > 0)
                        temp = temp.substr(0, temp.size() - 3) + "_" + std::to_string(i);
                    else
                        temp = temp + "_0";
                }
                else
                    break;
            }
            logName = temp;
        }
        void write(const std::string &msg)
        {
            this->msg += msg;
            if (this->msg.find("\n") != std::string::npos)
            {
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
                std::ofstream logFilet("log\\" + logName + ".log", std::ios::out | std::ios::app);
#endif
#if __linux__
                std::ofstream logFilet("/" + logName + "-log/" + logName + ".log", std::ios::out | std::ios::app);
#endif
                while (this->msg.find("\n") != std::string::npos)
                {
                    logFilet << getTime() << " " << this->msg.substr(0, this->msg.find("\n")) << std::endl;
                    this->msg = this->msg.substr(this->msg.find("\n") + 1);
                }
                logFilet.close();
            }
            mustChangeFlie();
            return;
        }
        void write(std::vector<std::string> &msg)
        {
            for (auto it = msg.begin(); it != msg.end(); ++it)
            {
                write(*it);
            }
            mustChangeFlie();
            return;
        }
        Log &operator<<(const std::string &msg)
        {
            while (this->writeFlie->exchange(true, std::memory_order_acquire))
                ;
            write(msg);
            this->writeFlie->store(false, std::memory_order_release);
            return *this;
        }
        Log &operator<<(const int msg)
        {
            while (this->writeFlie->exchange(true, std::memory_order_acquire))
                ;

            write(std::to_string(msg));
            this->writeFlie->store(false, std::memory_order_release);
            return *this;
        }
        funLog *getFunLog(const std::string name)
        {
            if (funlogList.find(name) == funlogList.end())
            {
                funlogList[name] = funLog(name, this);
            }
            return &funlogList[name];
        }

    private:
        std::string msg;
        int logMaxSize = 1024;
        std::atomic<bool> *writeFlie = new std::atomic<bool>(false);
        std::string getTime(void)
        {
            time_t now = time(NULL);
            tm *tm_t = localtime(&now);
            return ('[' + std::to_string(tm_t->tm_year + 1900) +
                    "/" + std::to_string(tm_t->tm_mon + 1) +
                    "/" + std::to_string(tm_t->tm_mday) +
                    ":" + std::to_string(tm_t->tm_hour) +
                    ":" + std::to_string(tm_t->tm_min) +
                    ":" + std::to_string(tm_t->tm_sec) + ']');
        }
        bool createDirectory(std::string folder)
        {
            std::string folder_builder;
            std::string sub;
            sub.reserve(folder.size());
            for (auto it = folder.begin(); it != folder.end(); ++it)
            {
                // cout << *(folder.end()-1) << endl;
                const char c = *it;
                sub.push_back(c);
                if (c == '\\' || it == folder.end() - 1)
                {
                    folder_builder.append(sub);

                    // this folder not exist
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
                    if (0 != ::_access(folder_builder.c_str(), 0))
                    {
                        if (0 != ::mkdir(folder_builder.c_str()))
#elif __linux__
                    if (0 != ::access(folder_builder.c_str(), 0))
                    {
                        if (0 != ::mkdir(folder_builder.c_str(), 777))
#endif
                        {
                            // create failed
                            return false;
                        }
                    }
                    sub.clear();
                }
            }
            return true;
        }
        std::ofstream logFile;
        std::string logName;
        std::map<std::string, funLog> funlogList;
    };
    void funLog::write(std::vector<std::string> &msg)
    {
        for (auto it = msg.begin(); it != msg.end(); ++it)
        {
            *it = this->name + " : " + *it;
        }
        this->log->write(msg);
        return;
    }
    void funLog::write(const std::string &msg)
    {
        log->write(this->name + " " + msg);
    }
    funLog &funLog::operator<<(const std::string &msg)
    {
        this->log->write(msg);
        return *this;
    }
    funLog &funLog::operator<<(const int &msg)
    {
        this->log->write(std::to_string(msg));
        return *this;
    }
};

#endif