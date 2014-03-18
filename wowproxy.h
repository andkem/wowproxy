#ifndef WOWPROXY_H
#define WOWPROXY_H

#include <QTcpServer>
#include <QNetworkProxy>
#include <QTcpSocket>
#include <QString>
#include <QThread>

#include <vector>

#include <thread>
#include <mutex>

namespace WoWProxy
{

struct Client
{
    QTcpSocket* to_socket;
    QTcpSocket* from_socket;

    std::mutex* mutex;

    QThread* to_thread;
    QThread* from_thread;
};

class networkthread : public QThread
{
    Q_OBJECT
public:
    networkthread(Client client, bool rev_data_direction, int thread_id, QObject* parent, void (*filter_function)(QByteArray&) = nullptr);

private:
    Client client;
    bool rev_data_direction;
    int thread_id;

    void (*FilterFunction)(QByteArray &data);

    void HandleClientData(Client client, bool rev_data_direction, int thread_id);

    void run();
};

class wowproxy : public QTcpServer
{
    Q_OBJECT
public:
    explicit wowproxy(QNetworkProxy server_proxy, qint16 listen_port, quint16 target_port, QString target_host, QObject *parent = 0, void (*filter_function)(QByteArray&) = nullptr);
    ~wowproxy();

private:
    QNetworkProxy proxy;

    qint16 listen_port;
    qint16 target_port;
    QString target_host_name;

    int next_thread_id;
    std::vector<Client> client_list;

    void incomingConnection(int socketDiscriptor);

    void (*FilterFunction)(QByteArray &data);

signals:
    
public slots:
    void HandleNewConnection();
    
};

}

#endif // WOWPROXY_H
