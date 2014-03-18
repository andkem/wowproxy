#include <QCoreApplication>
#include <QNetworkProxy>
#include "wowproxy.h"

#define WOWAUTHPORT 3724
#define REBIRTHPORT 8085
#define ADDRESS "137.117.101.20"

void filter(QByteArray &data)
{
    char org[] = "137.117.101.20";

    for (int i = 0; i < data.size() - 15; i++)
    {
        if (memcmp(org, data.constData() + i, 14) == 0)
        {
            qDebug() << (data.data() + i) << "\n";

            char new_addr[] = "127.000.000.01";
            for (unsigned int y = 0; y < 14; y++)
                data[i + y] = new_addr[y];

            qDebug() << (data.data() + i) << "\n";

        }
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
