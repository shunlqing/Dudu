#include "TCPClient.h"
#include <thread>

class Client;
class Session {
public:
    Session(EventLoop* loop, const std::string& peerAddr, const std::string& name, Client* owner);

    void Start() { client_.Connect(); }
    void Stop() { client_.DisConnect(); }

    int64_t bytes_read() const { return bytes_read_; }
    int64_t message_read() const { return messages_read_; }
    EventLoop* GetLoop() const { return client_.GetLoop(); }

private:
    TCPClient client_;
    Client* owner_;
    int64_t bytes_read_;
    int64_t bytes_written_;
    int64_t messages_read_;
    std::string name_;
};

class Client {
public:
    Client( const std::string& peerAddr,
            const std::string& name,
            int blockSize,
            int sessionCount,
            int timeout_sec,
            int threadCount)
            : session_count_(sessionCount),
              connected_count_(0),
              timeout_(timeout_sec),
              name_(name)

    {
        if(threadCount == 1) {
            loop_ = new EventLoop();
        } else {
            loop_ = new MultiEventLoop(threadCount);
        }

        for(int i = 0; i < blockSize; i++) {
            message_.push_back(static_cast<char>(i%128));
        }

        for(int i = 0; i < sessionCount; i++) {
            char buf[32];
            snprintf(buf, sizeof buf, "C%05d", i);
            Session* session = new Session(loop_->allocLoop(), peerAddr, buf, this);
            session->Start();
            sessions_.push_back(session);
        }
    }

    ~Client(){}

    const std::string& message() const { return message_; }

    void OnConnect() {
        if(++connected_count_ == session_count_) {
            std::cout << "all connected" << std::endl;
        }
    }

    void OnDisConnect() {
        if(--connected_count_ == 0) {
            std::cout << "all disconnected" << std::endl;

            int64_t totalByteRead = 0;
            int64_t totalMessageRead = 0;
            for(auto &it : sessions_) {
                totalByteRead += it->bytes_read();
                totalMessageRead += it->message_read();
            }

            std::cout << "name= " << name_ << " " << totalByteRead << " total bytes read" << std::endl;
            std::cout << "name= " << name_ << " " << totalMessageRead << " total messages read" << std::endl;
            std::cout << "name= " << name_ << " " << static_cast<double>(totalByteRead) / static_cast<double>(totalMessageRead) << " average message size" << std::endl;
            std::cout << "name= " << name_ << " " << static_cast<double>(totalByteRead) / (timeout_ * 1024 * 1024) << " Mib/s throughput" << std::endl;

            Quit();
        }
    }

    void Run() {
        loop_->runAfter(0, [this]{ HandleTimeout(); }, (int64_t)(timeout_ * 1000), 1);
        loop_->loop();
    }

private:
    void Quit() {
        loop_->Stop();
        for(auto &it : sessions_) {
            delete it;
        }

        sessions_.clear();
    }

    void HandleTimeout() {
//        std::cout << std::this_thread::get_id() << "stop" << std::endl;
        for(auto &it : sessions_) {
            auto lo = it->GetLoop();
            if(loop_ == lo)
                it->Stop();
            else
                lo->safeCall([&]{
                    it->Stop();
                });
        }
    }

private:
    EventLoop* loop_;
    std::string message_;
    std::string name_;
    int timeout_;
    std::vector<Session*> sessions_;
    int session_count_;
    std::atomic<int> connected_count_;

};

Session::Session(EventLoop *loop, const std::string& peerAddr, const std::string& name, Client *owner)
        : client_(loop, peerAddr),
          owner_(owner),
          bytes_read_(0),
          bytes_written_(0),
          messages_read_(0),
          name_(name)
{
    client_.OnConnect([this]{
        client_.GetConn().send(owner_->message().c_str(), owner_->message().size());
        owner_->OnConnect();
    });

    client_.OnMessage([this](const StreamSockPtr& ptr) {
        ++messages_read_;
        bytes_read_ += ptr->getInput().size();
        bytes_written_ += ptr->getInput().size();
        //std::cout << ptr->getInput().data() << std::endl;
        ptr->send(ptr->getInput());
    });
    client_.OnDisConnect([this]{
        owner_->OnDisConnect();
    });
}


int main(int argc, char* argv[])
{
    if(argc != 7) {
        fprintf(stderr, "Usage: client <host ip> <port> <threads> <blocksize> <sessions> <time_seconds>\n");
        return -1;
    }

//    argv[1] = "127.0.0.1";
//    argv[2] = "8088";
//    argv[3] = "4";
//    argv[4] = "16384";
//    argv[5] = "100";
//    argv[6] = "10";

    const char* ip = argv[1];
    uint16_t port = static_cast<uint16_t>(atoi(argv[2]));
    int threadCount = atoi(argv[3]);
    int blockSize = atoi(argv[4]);
    int sessionCount = atoi(argv[5]);
    int timeout = atoi(argv[6]);

    std::string serverAddr = std::string(ip) + ":" + std::to_string(port);
    Client client(serverAddr, argv[0], blockSize, sessionCount, timeout, threadCount);
    client.Run();
}

