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

   class QtNetworkCookieJar;

   class QtHttpClient: public QObject {

      Q_OBJECT

      public:
         QtHttpClient (const PluginInfo &Info, QObject *parent = 0);
         virtual ~QtHttpClient();

         QNetworkRequest get_next_request (const QUrl &Url);

         virtual Boolean is_request_pending (const UInt64 RequestId) const;

         virtual QNetworkReply *get_reply (const UInt64 RequestId) const;

         virtual UInt64 get (const QUrl &Url);
         virtual UInt64 get (const QNetworkRequest &Request);

         virtual UInt64 put (const QUrl &Url, const QByteArray &Data = "");
         virtual UInt64 put (const QNetworkRequest &Request, const QByteArray &Data);

         virtual UInt64 del (const QUrl &Url);
         virtual UInt64 del (const QNetworkRequest &Request);

         virtual UInt64 post (
            const QUrl &Url,
            const QMap<QString, QString> &Params);

         virtual UInt64 post (
            const QNetworkRequest &Request,
            const QMap<QString, QString> &Params);

         virtual UInt64 post (const QUrl &Url, const QByteArray &Data = "");

         virtual UInt64 post (
            const QNetworkRequest &Request,
            const QByteArray &Data);

   Q_SIGNALS:
         void reply_aborted (const UInt64 RequestId);
         void reply_error (const UInt64 RequestId, const QString &ErrorMessage);
         void reply_finished (const UInt64 RequestId, QNetworkReply *reply);

         void reply_download_progress (
            const UInt64 RequestId,
            QNetworkReply *reply,
            qint64 bytesReceived,
            qint64 bytesTotal);

         void authentication_required (
            QNetworkReply *reply,
            QAuthenticator *authenticator);

         void authentication_error (const UInt64 RequestId);

      public Q_SLOTS:
         void abort (const UInt64 RequestId);
         void abort_all ();

      protected Q_SLOTS:
         void _reply_download_progress (qint64 bytesReceived, qint64 bytesTotal);
         void _reply_finished ();
         void _reply_error ();
         void _ssl_errors (QNetworkReply *reply, const QList<QSslError> &Errors);

      protected:
         typedef QMap<QString, QString> StringMap;

         QNetworkReply *_do_request (
               const QString &Method,
               const QNetworkRequest &request,
               const QByteArray &Data = "");

         void _add_headers (
            QNetworkRequest &request,
            const QMap<QByteArray, QByteArray> &Headers);

         UInt64 _get_request_id (QNetworkReply *reply) const;

      protected:
         Log _log;
         RuntimeContext *_context;
         QtNetworkCookieJar *_cookieJar;
         QNetworkAccessManager *_manager;
         UInt64 _requestCounter;
         QMap<UInt64, QNetworkReply *> _replyMap;
         QMap<QByteArray, QByteArray> _defaultHeaders;
   };
};


#endif // DMZ_QT_HTTP_CLIENT_DOT_H
