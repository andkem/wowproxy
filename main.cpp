#include <QCoreApplication>
#include <QNetworkProxy>
#include "wowproxy.h"

#define WOWAUTHPORT 3724
#define REBIRTHPORT 8085
#define ADDRESS "137.117.100.211"
#define REPLACEMENT_ADDRESS "127.000.000.001:8085"

#define ADDRESS_OFFSET 21

void filter(QByteArray &data)
{
    char org[]         = ADDRESS;
    char new_address[] = REPLACEMENT_ADDRESS;

    if (data.contains(org))
    {
        qDebug() << "Replaced: " << &data.data()[21];
        data.replace(ADDRESS_OFFSET, sizeof(org), new_address, sizeof(org));
        qDebug() << " with: " << &data.data()[21] << "\n";
    }
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    QNetworkProxy proxy;
    proxy.setType(QNetworkProxy::Socks5Proxy);
    proxy.setHostName("127.0.0.1");
    proxy.setPort(8080);

    WoWProxy::wowproxy auth_proxy(proxy, WOWAUTHPORT, WOWAUTHPORT, QString(ADDRESS), 0, &filter);
    WoWProxy::wowproxy data_proxy(proxy, REBIRTHPORT, REBIRTHPORT, QString(ADDRESS));

    return a.exec();
}
