#pragma once

#include <iostream>
#include <functional>
#include <fstream>
#include <array>
#include <memory>
#include <future>

#include <boost/asio.hpp>

using handle_finished_t = std::function<void()>;
using before_saving_t = std::function<void(std::string)>;
class Confs;

class AsyncTcpClient {
public:
    AsyncTcpClient(boost::asio::io_service &io_service,
        const std::string &serverIp,
        const std::string &serverPort,
        const std::string &path,
        const std::string &path_to,
        handle_finished_t finishedCb,
        std::promise<int> *prom
        );

private:
    void handleResolve(const boost::system::error_code &err,
        boost::asio::ip::tcp::resolver::iterator endpointIterator);
    
    void handleConnect(const boost::system::error_code &err,
        boost::asio::ip::tcp::resolver::iterator endpointIterator);
    
    void handleWriteFile(const boost::system::error_code &err);

private:
    handle_finished_t mFinishedCb;
    std::promise<int> *mPromise;
    boost::asio::ip::tcp::resolver mResolver;
    boost::asio::ip::tcp::socket mSocket;
    std::array<char, 1024> mBuf;
    boost::asio::streambuf mRequest;
    std::ifstream mSourceFile;
};

class AsyncTcpConnection : public std::enable_shared_from_this<AsyncTcpConnection> {
public:
    AsyncTcpConnection(boost::asio::io_service &io_service, handle_finished_t finishedCb, before_saving_t bfs);
    void start();
    boost::asio::ip::tcp::socket &socket();

private:
    void handleReadRequest(const boost::system::error_code &err, size_t transferredBytes);

    void handleReadFileContent(const boost::system::error_code &err, size_t transferredBytes);

    void handleError(const std::string &functionName, const boost::system::error_code &err);

private:
    boost::asio::streambuf mRequestBuf;
    std::ofstream mOutputFile;
    boost::asio::ip::tcp::socket mSocket;
    size_t mFileSize;
    handle_finished_t mFinishedCb;
    std::array<char, 40960> mBuf;
    before_saving_t mBeforeSaving;
};

class AsyncTcpServer : private boost::noncopyable {
public:
    typedef std::shared_ptr<AsyncTcpConnection> ConnectionPtr;

    AsyncTcpServer(unsigned int port, handle_finished_t finishedCb, before_saving_t bfs);
    ~AsyncTcpServer();

    void handleAccept(ConnectionPtr currentConnection, const boost::system::error_code &e);

private:
    boost::asio::io_service mIoService;
    std::shared_ptr<boost::asio::io_service::work> mWork;
    boost::asio::ip::tcp::acceptor mAcceptor;
    handle_finished_t mFinishedCb;
    before_saving_t mBeforeSaving;
    std::thread mThread;
};
