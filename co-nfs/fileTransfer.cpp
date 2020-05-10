#include "co-nfs/fileTransfer.h"

#include <iostream>
#include <iostream>
#include <functional>
#include <fstream>
#include <array>
#include <memory>

#include <boost/asio.hpp>
#include <boost/bind.hpp>

using namespace std;
using tcp = boost::asio::ip::tcp;
namespace asio = boost::asio;

AsyncTcpClient::AsyncTcpClient(asio::io_service &io_service,
    const string &serverIp,
    const string &serverPort,
    const std::string &path,
    const std::string &path_to,
    handle_finished_t finishedCb,
    promise<int> *prom)
    : mResolver(io_service)
    , mSocket(io_service)
    , mFinishedCb(finishedCb)
    , mPromise(prom)
{
    mSourceFile.open(path, ios_base::binary|ios_base::ate);
    if (!mSourceFile) {
        cout << __LINE__ << "Failed to open " << path << endl;
        mPromise->set_value(-1);
        return;
    }
    size_t fileSize = mSourceFile.tellg();
    mSourceFile.seekg(0);
    ostream requestSteam(&mRequest);
    requestSteam << path_to << "\n" << fileSize << "\n\n";
    cout << "Request size: " << mRequest.size() << " path_to:" << path_to << endl;
    tcp::resolver::query query(serverIp, serverPort);
    mResolver.async_resolve(query, 
        boost::bind(&AsyncTcpClient::handleResolve, this, 
            asio::placeholders::error, 
            asio::placeholders::iterator));
}

void AsyncTcpClient::handleResolve(const boost::system::error_code &err, tcp::resolver::iterator endpointIterator)
{
    if (!err) {
        tcp::endpoint endpoint = *endpointIterator;
        mSocket.async_connect(endpoint, 
            boost::bind(&AsyncTcpClient::handleConnect, this,
                asio::placeholders::error,
                ++endpointIterator));
    }
    else {
        mSourceFile.close();
        mPromise->set_value(-1);
        cout << "Error: " << err.message() << '\n';
    }
}

void AsyncTcpClient::handleConnect(const boost::system::error_code &err, tcp::resolver::iterator endpointIterator)
{
    if (!err) {
        asio::async_write(mSocket, 
            mRequest, 
            boost::bind(&AsyncTcpClient::handleWriteFile, this, asio::placeholders::error));
    }
    else if (endpointIterator != tcp::resolver::iterator()) {
        mSocket.close();
        tcp::endpoint endpoint = *endpointIterator;
        mSocket.async_connect(endpoint, 
            boost::bind(&AsyncTcpClient::handleConnect, this,
                asio::placeholders::error,
                ++endpointIterator));

    }
    else {
        mSourceFile.close();
        mPromise->set_value(-1);
        cout << "Error:" << err.message() << '\n';
    }
}

void AsyncTcpClient::handleWriteFile(const boost::system::error_code &err)
{
    if (!err) {
        if (mSourceFile) {
            mSourceFile.read(mBuf.data(), static_cast<streamsize>(mBuf.size()));
            if (mSourceFile.gcount() <= 0) {
                mSourceFile.close();
                cout << "read file error" << endl;
                mPromise->set_value(0);
                return;
            }
            cout << "Send " << mSourceFile.gcount() << "bytes, total:" << mSourceFile.tellg() << "bytes.\n";
            asio::async_write(mSocket,
                asio::buffer(mBuf.data(), mSourceFile.gcount()),
                boost::bind(&AsyncTcpClient::handleWriteFile, this, asio::placeholders::error));
        }
        else {
            mSourceFile.close();
            mPromise->set_value(0);
            mFinishedCb();
            return;
        }
    }
    else {
        mSourceFile.close();
        mPromise->set_value(-1);
        cout << "Error: " << err.message() << '\n';
    }
}

AsyncTcpConnection::AsyncTcpConnection(asio::io_service &io_service, handle_finished_t finishedCb, before_saving_t bfs)
    : mSocket(io_service), mFileSize(0), mFinishedCb(finishedCb), mBeforeSaving(bfs)
{
}

void AsyncTcpConnection::start()
{
    asio::async_read_until(mSocket, mRequestBuf, "\n\n", 
        boost::bind(&AsyncTcpConnection::handleReadRequest,
            shared_from_this(),
            asio::placeholders::error,
            asio::placeholders::bytes_transferred));
}

tcp::socket &AsyncTcpConnection::socket()
{
    return mSocket;
}

void AsyncTcpConnection::handleReadRequest(const boost::system::error_code &err, size_t transferredBytes)
{
    if (err) {
        return handleError(__FUNCTION__, err);
    }
    istream requestStream(&mRequestBuf);
    string filePath;
    requestStream >> filePath;
    mBeforeSaving(filePath);
    requestStream >> mFileSize;
    requestStream.read(mBuf.data(), 2);
    mOutputFile.open(filePath, ios_base::binary);
    if (!mOutputFile) {
        cout << __LINE__ << "Failed to open: " << filePath << " size:" << mFileSize << endl;
        return;
    }
    do {
        requestStream.read(mBuf.data(), static_cast<streamsize>(mBuf.size()));
        mOutputFile.write(mBuf.data(), requestStream.gcount());
    } while(requestStream.gcount() > 0);
    asio::async_read(mSocket, asio::buffer(mBuf.data(), mBuf.size()),
        boost::bind(&AsyncTcpConnection::handleReadFileContent,
            shared_from_this(),
            asio::placeholders::error,
            asio::placeholders::bytes_transferred));
}

void AsyncTcpConnection::handleReadFileContent(const boost::system::error_code &err, size_t transferredBytes)
{
    if (transferredBytes > 0 || err == boost::asio::error::eof) {
        mOutputFile.write(mBuf.data(), static_cast<streamsize>(transferredBytes));
        if (mOutputFile.tellp() >= static_cast<streamsize>(mFileSize)) {
            cout << __FUNCTION__ << " finished." << endl;
            mOutputFile.close();
            mFinishedCb();
            return;
        }
    }
    if (err) {
        mOutputFile.close();
        return handleError(__FUNCTION__, err);
    }
    asio::async_read(mSocket, asio::buffer(mBuf.data(), mBuf.size()),
        boost::bind(&AsyncTcpConnection::handleReadFileContent,
            shared_from_this(),
            asio::placeholders::error,
            asio::placeholders::bytes_transferred));
}

void AsyncTcpConnection::handleError(const string &functionName, const boost::system::error_code &err)
{
    cout << __FUNCTION__ << " in " << functionName << " due to " << err << " " << err.message() << endl;
}

AsyncTcpServer::AsyncTcpServer(unsigned int port, handle_finished_t finishedCb, before_saving_t bfs)
    : mAcceptor(mIoService, tcp::endpoint(tcp::v4(), port), true), mFinishedCb(finishedCb), mBeforeSaving(bfs)
{
    ConnectionPtr newConnection(new AsyncTcpConnection(mIoService, finishedCb, bfs));

    mAcceptor.listen();
    mAcceptor.async_accept(newConnection->socket(), 
        boost::bind(&AsyncTcpServer::handleAccept, this,
        newConnection,
        asio::placeholders::error));
    mWork = make_shared<asio::io_service::work>(asio::io_service::work(mIoService));
    //mThread = thread([&](){ mIoService.run(); });
    mIoService.run();
}

void AsyncTcpServer::handleAccept(ConnectionPtr currentConnection, const boost::system::error_code &err)
{
    if (!err) {
        cout << "!!!!!!!!!!!!!!!!!" << "new connection" << endl;
        currentConnection->start();
        ConnectionPtr newConnection(new AsyncTcpConnection(mIoService, mFinishedCb, mBeforeSaving));
        mAcceptor.async_accept(newConnection->socket(), 
            boost::bind(&AsyncTcpServer::handleAccept, this,
            newConnection,
            asio::placeholders::error));
    }
}

AsyncTcpServer::~AsyncTcpServer()
{
    mIoService.stop();
}
