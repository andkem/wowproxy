#include "wowproxy.h"
#include <iostream>


namespace WoWProxy
{

networkthread::networkthread(Client client, bool rev_data_direction, int thread_id, QObject* parent, void (*filter_function)(QByteArray&)) :
    QThread(parent), client(client), rev_data_direction(rev_data_direction), thread_id(thread_id), FilterFunction(filter_function)
{

}

void networkthread::run()
{
    HandleClientData(client, rev_data_direction, thread_id);
}

void networkthread::HandleClientData(Client client, bool rev_data_direction, int thread_id)
{
    QTcpSocket *from, *to;
    std::mutex *mutex;

    if (rev_data_direction)
    {
        from = client.to_socket;
        mutex = client.mutex;
    }
    else
    {
        from = client.from_socket;
        mutex = client.mutex;
    }

    to = client.to_socket;

    qDebug() << "Network thread " << thread_id << " started!\n";

    while(from->state() == QAbstractSocket::ConnectedState && to->state() == QAbstractSocket::ConnectedState)
    {
        if (from->waitForReadyRead())
        {
            mutex->lock();
            QByteArray result = from->readAll();

            if (FilterFunction)
                (*FilterFunction)(result);

            if (to->state() == QAbstractSocket::ConnectedState)
            {
                to->write(result);
                to->flush();
            }
            else
            {
                from->disconnectFromHost();
                mutex->unlock();
                break;
            }
        }

        mutex->unlock();

    }

    qDebug() << "Network thread " << thread_id << " finished!\n";
}

wowproxy::wowproxy(QNetworkProxy server_proxy, qint16 listen_port, quint16 target_port, QString target_host, QObject *parent, void (*filter_function)(QByteArray&)) :
    QTcpServer(parent), proxy(server_proxy), listen_port(listen_port), target_port(target_port), target_host_name(target_host), FilterFunction(filter_function)
{
    connect(this, SIGNAL(newConnection()), SLOT(HandleNewConnection()));

    listen(QHostAddress::Any, listen_port);

    next_thread_id = 0;
}

wowproxy::~wowproxy()
{
    for (Client client : client_list)
    {
        if (client.to_socket->state() == QAbstractSocket::ConnectedState)
            client.to_socket->disconnectFromHost();


        delete client.to_socket;
        delete client.from_socket;
        delete client.mutex;
        delete client.to_thread;
        delete client.from_thread;
    }
}

void wowproxy::incomingConnection(int socketDescriptor)
{
    QTcpSocket *client = new QTcpSocket();
    client->setSocketDescriptor(socketDescriptor);

    qDebug() << "Handling new connection!\n";

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

    std::mutex* mutex = new std::mutex;

    Client new_client = { server_socket, client, mutex, NULL, NULL };


    networkthread *to = new networkthread(new_client, false, next_thread_id++, this, FilterFunction);
    to->start();

    networkthread *from = new networkthread(new_client, true, next_thread_id++, this, FilterFunction);
    from->start();

    client->moveToThread(to);
    server_socket->moveToThread(from);

    new_client.to_thread = to;
    new_client.from_thread = from;

    client_list.push_back(new_client);

    auto itr = client_list.begin();
    do
    {
        if (!itr->to_thread->isRunning() && !itr->from_thread->isRunning())
        {
            delete itr->to_socket;
            delete itr->from_socket;
            delete itr->mutex;
            delete itr->to_thread;
            delete itr->from_thread;

            itr = client_list.erase(itr);
        }
        else
            ++itr;
    } while(itr != client_list.end());

}

void wowproxy::HandleNewConnection()
{

}
}
