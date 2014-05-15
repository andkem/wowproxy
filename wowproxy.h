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
    QTcpSocket* to_socket;       // The socket being written to in the network thread.
    QTcpSocket* from_socket;     // The socket being read from in the network thread.

    std::mutex* mutex;

    QThread* to_thread;         // The thread that writes to the server and reads from the client.
    QThread* from_thread;       // The thread that has the data direction reversed and writes to the client and reads from the server.
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

    void (*filter_function)(QByteArray &data);

    void handle_client_data(Client client, bool rev_data_direction, int thread_id);

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

    void (*filter_function)(QByteArray &data);

signals:
    
public slots:
    void handle_new_connection();
    
};

}

#endif // WOWPROXY_H
