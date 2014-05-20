#include "wowproxy.h"
#include <iostream>


namespace WoWProxy
{

networkthread::networkthread(Client client, int thread_id, QObject* parent, void (*filter_function)(QByteArray&)) :
    QThread(parent), client(client), thread_id(thread_id), filter_function(filter_function)
{

}

void networkthread::run()
{
    handle_client_data(client, thread_id);
}

void networkthread::handle_client_data(Client client, int thread_id)
{
    qDebug() << "Network thread " << thread_id << " started!\n";

    while(client.from_socket->state() == QAbstractSocket::ConnectedState && client.to_socket->state() == QAbstractSocket::ConnectedState)
    {

        if (client.from_socket->waitForReadyRead(1))
        {
            QByteArray result = client.from_socket->readAll();

            if (filter_function)
                (*filter_function)(result);

            if (client.to_socket->state() == QAbstractSocket::ConnectedState)
            {
                client.to_socket->write(result);
                client.to_socket->flush();
            }
            else
            {
                client.from_socket->disconnectFromHost();
                break;
            }
        }

        if (client.to_socket->waitForReadyRead(1))
        {
            QByteArray result = client.to_socket->readAll();

            if (filter_function)
                (*filter_function)(result);

            if (client.from_socket->state() == QAbstractSocket::ConnectedState)
            {
                client.from_socket->write(result);
                client.from_socket->flush();
            }
            else
            {
                client.to_socket->disconnectFromHost();
                break;
            }
        }

    }

    qDebug() << "Network thread " << thread_id << " finished!\n";
}

wowproxy::wowproxy(QNetworkProxy server_proxy, qint16 listen_port, quint16 target_port, QString target_host, QObject *parent, void (*filter_function)(QByteArray&)) :
    QTcpServer(parent), proxy(server_proxy), listen_port(listen_port), target_port(target_port), target_host_name(target_host), filter_function(filter_function)
{
    connect(this, SIGNAL(newConnection()), SLOT(handle_new_connection()));

    listen(QHostAddress::Any, listen_port);

    next_thread_id = 0;
}

wowproxy::~wowproxy()
{
    cleanup_clients(true);
}

void wowproxy::incomingConnection(int socketDescriptor)
{
    QTcpSocket *client_socket = new QTcpSocket();
    client_socket->setSocketDescriptor(socketDescriptor);

    qDebug() << "Handling new connection!\n";

    QTcpSocket *server_socket = new QTcpSocket();
    server_socket->setProxy(proxy);
    server_socket->connectToHost(target_host_name, target_port);
    if (!server_socket->waitForConnected())
    {
        qDebug() << server_socket->errorString();

        client_socket->disconnectFromHost();

        delete server_socket;
        delete client_socket;

        return;
    }

    Client new_client = { server_socket, client_socket, NULL };

    networkthread *client_thread = new networkthread(new_client, next_thread_id++, this, filter_function);
    client_thread->start();

    client_socket->moveToThread(client_thread);
    server_socket->moveToThread(client_thread);

    new_client.client_thread = client_thread;

    client_list.push_back(new_client);

    cleanup_clients(false);
}

void wowproxy::cleanup_clients(bool wait)
{
    auto itr = client_list.begin();
    do
    {
        if (itr->client_thread->wait(wait ? ULONG_MAX : 1))
        {
            if (itr->to_socket->state() == QAbstractSocket::ConnectedState)
                itr->to_socket->disconnectFromHost();

            if (itr->from_socket->state() == QAbstractSocket::ConnectedState)
                itr->from_socket->disconnectFromHost();

            delete itr->to_socket;
            delete itr->from_socket;
            delete itr->client_thread;

            itr = client_list.erase(itr);
        }
        else
            ++itr;
    } while(itr != client_list.end());
}

void wowproxy::handle_new_connection()
{

}
}
