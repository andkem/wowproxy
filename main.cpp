#include <QCoreApplication>
#include <QNetworkProxy>
#include "wowproxy.h"

#define WOWAUTHPORT 3724
#define REBIRTHPORT 8085
#define WEBPORT 8081

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    QNetworkProxy proxy;
    proxy.setType(QNetworkProxy::Socks5Proxy);
    proxy.setHostName("127.0.0.1");
    proxy.setPort(8080);

    WoWProxy::wowproxy auth_proxy(proxy, WOWAUTHPORT, WOWAUTHPORT, QString("wow.therebirth.net"));
    WoWProxy::wowproxy data_proxy(proxy, REBIRTHPORT, REBIRTHPORT, QString("wow.theribirth.net"));
    
    return a.exec();
}
