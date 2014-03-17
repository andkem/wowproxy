#include "wowproxy.h"
#include <iostream>


namespace WoWProxy
{

networkthread::networkthread(Client client, bool rev_data_direction, int thread_id, QObject* parent) :
    QThread(parent), client(client), rev_data_direction(rev_data_direction), thread_id(thread_id)
{

}

void networkthread::run()
{
    HandleClientData(client, rev_data_direction, thread_id);
}

void networkthread::HandleClientData(Client client, bool rev_data_direction, int thread_id)
{
    unsigned long buffer_size = 1500;
    char *buffer = new char[buffer_size];

    QTcpSocket *from, *to;
    std::mutex *from_mutex, *to_mutex;

    if (rev_data_direction)
    {
        from = client.to_socket;
        from_mutex = client.to_mutex;

        to = client.from_socket;
        to_mutex = client.from_mutex;
    }
    else
    {
        from = client.from_socket;
        from_mutex = client.from_mutex;

        to = client.to_socket;
        to_mutex = client.to_mutex;
    }

    std::cout << "Network thread " << thread_id << " started!\n";

    while(from->state() == QAbstractSocket::ConnectedState && to->state() == QAbstractSocket::ConnectedState)
    {
        from_mutex->lock();
        unsigned long available_client_data = from->bytesAvailable();

        if (available_client_data > 0)//client->waitForReadyRead(1) || server_socket->waitForReadyRead(1))
        {
            if (available_client_data > buffer_size)
            {
                delete[] buffer;

                buffer_size = available_client_data;

                buffer = new char[available_client_data];
            }

            if (available_client_data > 0 && from->read(buffer, available_client_data))
            {
                to_mutex->lock();
                to->write(buffer, available_client_data);
                to->flush();
                to_mutex->unlock();
            }
        }
        from_mutex->unlock();

        msleep(1);
    }

    delete[] buffer;

    std::cout << "Network thread " << thread_id << " finished!\n";
}

wowproxy::wowproxy(QNetworkProxy server_proxy, qint16 listen_port, quint16 target_port, QString target_host, QObject *parent) :
    QTcpServer(parent), listen_port(listen_port), target_port(target_port)
{
    connect(this, SIGNAL(newConnection()), SLOT(HandleNewConnection()));

    listen(QHostAddress::Any, listen_port);

    proxy = server_proxy;
    target_host_name = target_host;

    next_thread_id = 0;
}

wowproxy::~wowproxy()
{
    for (Client client : client_list)
    {
        if (client.to_socket->state() == QAbstractSocket::ConnectedState)
            client.to_socket->disconnectFromHost();


        delete client.to_socket;
        delete client.to_mutex;
        delete client.from_mutex;
        delete client.to_thread;
        delete client.from_thread;
    }
}

void wowproxy::HandleNewConnection()
{
    QTcpSocket *client;

    while ((client = nextPendingConnection()) != 0)
    {
        std::cout << "Handling new connection!\n";

        QTcpSocket *server_socket = new QTcpSocket();
        server_socket->setProxy(proxy);
        server_socket->connectToHost(target_host_name, target_port);
        if (!server_socket->waitForConnected())
        {
            qDebug() << server_socket->errorString();

            client->disconnectFromHost();

            delete server_socket;

            return;
        }

        std::mutex* to_mutex = new std::mutex;
        std::mutex* from_mutex = new std::mutex;


        Client new_client = { server_socket, client, to_mutex, from_mutex, NULL, NULL };


        networkthread *to = new networkthread(new_client, false, next_thread_id++, this);
        to->start();

        networkthread *from = new networkthread(new_client, true, next_thread_id++, this);
        from->start();

        new_client.to_thread = to;
        new_client.from_thread = from;

        client_list.push_back(new_client);
    }
}
}
