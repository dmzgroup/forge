#ifndef DMZ_QT_NETWORK_COOKIE_JAR_DOT_H
#define DMZ_QT_NETWORK_COOKIE_JAR_DOT_H

#include <dmzRuntimeConfig.h>
#include <QtNetwork/QNetworkCookieJar>

namespace dmz {

   class QtNetworkCookieJar : public QNetworkCookieJar {

      Q_OBJECT

      public:
         QtNetworkCookieJar (QObject *parent = 0);
         ~QtNetworkCookieJar ();

         void save_cookies (Config &session);
         void load_cookies (const Config &Session);
   };
};

#endif // DMZ_QT_NETWORK_COOKIE_JAR_DOT_H
