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

    qDebug() << "Network thread " << thread_id << " started!\n";

    while(from->state() == QAbstractSocket::ConnectedState && to->state() == QAbstractSocket::ConnectedState)
    {
        from_mutex->lock();

        if (from->waitForReadyRead(0)) //available_client_data > 0)
        {
            QByteArray result = from->readAll();

            char org[] = "137.117.101.20";

            for (int i = 0; i < result.size() - 15; i++)
            {
                if (memcmp(org, result.data() + i, 14) == 0)
                {
                    qDebug() << (result.data() + i) << "\n";

                    char new_addr[] = "127.000.000.01";
                    for (unsigned int y = 0; y < 14; y++)
                        result[i + y] = new_addr[y];

                    qDebug() << (result.data() + i) << "\n";

                }
            }


            to_mutex->lock();
            to->write(result);
            to->flush();
            to_mutex->unlock();
        }
        from_mutex->unlock();

        usleep(100);
    }

    qDebug() << "Network thread " << thread_id << " finished!\n";
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
        delete client.from_socket;
        delete client.to_mutex;
        delete client.from_mutex;
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

    std::mutex* to_mutex = new std::mutex;
    std::mutex* from_mutex = new std::mutex;


    Client new_client = { server_socket, client, to_mutex, from_mutex, NULL, NULL };


    networkthread *to = new networkthread(new_client, false, next_thread_id++, this);
    to->start();

    networkthread *from = new networkthread(new_client, true, next_thread_id++, this);
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
            delete itr->to_mutex;
            delete itr->from_mutex;
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
