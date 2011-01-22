#ifndef DMZ_QT_HTTP_CLIENT_DOT_H
#define DMZ_QT_HTTP_CLIENT_DOT_H

#include <dmzRuntimeLog.h>
#include <dmzRuntimePluginInfo.h>
#include <QtCore/QMap>
#include <QtCore/QObject>
#include <QtNetwork/QAuthenticator>
#include <QtNetwork/QNetworkReply>

class QByteArray;
class QNetworkAccessManager;
class QUrl;


namespace dmz {


   class QtHttpClient: public QObject {
      
      Q_OBJECT
            
      public:
         QtHttpClient (const PluginInfo &Info, QObject *parent = 0);
         virtual ~QtHttpClient();

         virtual Boolean is_request_pending (const UInt64 RequestId) const;

         virtual QString get_username() const;
         virtual QString get_password() const;

         virtual UInt64 get (const QUrl &Url);
         virtual UInt64 put (const QUrl &Url, const QByteArray &Data = "");
         virtual UInt64 del (const QUrl &Url);

      Q_SIGNALS:
         void reply_aborted (const UInt64 RequestId);

         void reply_error (
            const UInt64 RequestId,
            const QString &ErrorMessage,
            const QNetworkReply::NetworkError Error);

         void reply_finished (const UInt64 RequestId, QNetworkReply *reply);
         
         void authentication_error (const UInt64 RequestId);

      public Q_SLOTS:
         void update_username (const QString &UserName, const QString &Password);
         void abort (const UInt64 RequestId);
         void abort_all ();

      protected Q_SLOTS:
         void _authenticate (QNetworkReply *reply, QAuthenticator *authenticator);
         void _reply_finished ();
         void _reply_error ();
         void _ssl_errors (QNetworkReply *reply, const QList<QSslError> &Errors);

      protected:
         QNetworkReply *_request (
               const QString &Method,
               const QUrl &Url,
               const UInt64 RequestId,
               const QByteArray &Data = "");
         
         UInt64 _get_request_id (QNetworkReply *reply) const;
         
         Boolean _add_basic_auth_header (QNetworkRequest &request);
         
      protected:
         Log _log;
         QNetworkAccessManager *_manager;
         QAuthenticator _auth;
         
         UInt64 _requestCounter;
         QMap<UInt64, QNetworkReply *> _replyMap;
   };
};


#endif // DMZ_QT_HTTP_CLIENT_DOT_H
