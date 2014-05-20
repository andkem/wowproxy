#ifndef WOWPROXY_H
#define WOWPROXY_H

#include <QTcpServer>
#include <QNetworkProxy>
#include <QTcpSocket>
#include <QString>
#include <QThread>

#include <list>

#include <thread>
#include <mutex>

namespace WoWProxy
{

struct Client
{
    QTcpSocket* to_socket;       // The socket connected to the remote server.
    QTcpSocket* from_socket;     // The socket connected to the local client.

    QThread* client_thread;
};

class networkthread : public QThread
{
    Q_OBJECT
public:
    networkthread(Client client, int thread_id, QObject* parent, void (*filter_function)(QByteArray&) = nullptr);

private:
    Client client;
    int thread_id;

    void (*filter_function)(QByteArray &data);

    void handle_client_data(Client client, int thread_id);

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
    std::list<Client> client_list;

    void incomingConnection(int socketDiscriptor);
    void cleanup_clients(bool wait);

    void (*filter_function)(QByteArray &data);

signals:
    
public slots:
    void handle_new_connection();
    
};

}

#endif // WOWPROXY_H
