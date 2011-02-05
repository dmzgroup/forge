#include "dmzQtHttpClient.h"
#include <QtCore/QUrl>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QSslError>
#include <QtCore/QDebug>


namespace {

   static const QString LocalGet ("get");
   static const QString LocalPut ("put");
   static const QString LocalPost ("post");
   static const QString LocalDelete ("delete");

   static const char LocalUserAgentName[] = "dmz-qt-http-client/0.1";
   static const char LocalAccept[] = "Accept";
   static const char LocalApplicationJson[] = "application/json";
   static const char LocalApplicationTextPlain[] = "text/plain; charset=utf-8";
   static const char LocalApplicationFormUrlEncoded[] = "application/x-www-form-urlencoded";
   static const char LocalApplicationOctetStream[] = "application/octet-stream";
   static const char LocalUserAgent[] = "User-Agent";
   static const char LocalETag[]= "ETag";
   static const char LocalIfNoneMatch[] = "If-None-Match";

   static QNetworkRequest::Attribute LocalAttrId =
      (QNetworkRequest::Attribute) (QNetworkRequest::User + 2);
};


dmz::QtHttpClient::QtHttpClient (const PluginInfo &Info, QObject *parent) :
      QObject (parent),
      _log (Info),
      _requestCounter (1000) {

   _manager = new QNetworkAccessManager (this);

   connect (
      _manager, SIGNAL (authenticationRequired (QNetworkReply *, QAuthenticator *)),
      this, SIGNAL (authentication_required (QNetworkReply *, QAuthenticator *)));

   connect (
      _manager, SIGNAL (sslErrors (QNetworkReply *, const QList<QSslError> &)),
      this, SLOT (_ssl_errors (QNetworkReply *, const QList<QSslError> &)));

//   connect (
//      _manager, SIGNAL (authenticationRequired (QNetworkReply *, QAuthenticator *)),
//      this, SLOT (_authenticate (QNetworkReply *, QAuthenticator *)));
}


dmz::QtHttpClient::~QtHttpClient () {

   if (_manager) {

      abort_all ();
      _manager->deleteLater ();
      _manager = 0;
   }
}


dmz::Boolean
dmz::QtHttpClient::is_request_pending (const UInt64 RequestId) const {

   return _replyMap.contains (RequestId);
}


QNetworkReply *
dmz::QtHttpClient::get_reply (const UInt64 RequestId) const {

   return _replyMap.value (RequestId);
}


dmz::UInt64
dmz::QtHttpClient::get (const QUrl &Url) {

   UInt64 requestId (0);

   if (Url.isValid ()) {

      requestId = _requestCounter++;

      QNetworkReply *reply = _request (LocalGet, Url, requestId);
   }

   return requestId;
}


dmz::UInt64
dmz::QtHttpClient::put (const QUrl &Url, const QByteArray &Data) {

   UInt64 requestId (0);

   if (Url.isValid ()) {

      requestId = _requestCounter++;

      QNetworkReply *reply = _request (LocalPut, Url, requestId, Data);
   }

   return requestId;
}


dmz::UInt64
dmz::QtHttpClient::post (const QUrl &Url, const QMap<QString, QString> &Params) {

   QUrl postData;
   QMapIterator<QString, QString> it (Params);

   while (it.hasNext ()) {

      it.next();
      postData.addQueryItem (it.key (), it.value ());
   }

   return post (Url, postData.encodedQuery());
}


dmz::UInt64
dmz::QtHttpClient::post (const QUrl &Url, const QByteArray &Data) {

   UInt64 requestId (0);

   if (Url.isValid ()) {

      requestId = _requestCounter++;

      QNetworkReply *reply = _request (LocalPost, Url, requestId, Data);
   }

   return requestId;
}


dmz::UInt64
dmz::QtHttpClient::del (const QUrl &Url) {

   UInt64 requestId (0);

   if (Url.isValid ()) {

      requestId = _requestCounter++;

      QNetworkReply *reply = _request (LocalDelete, Url, requestId);
   }

   return requestId;
}


void
dmz::QtHttpClient::abort (const UInt64 RequestId) {

   if (_replyMap.contains (RequestId)) {

      QNetworkReply *reply = _replyMap.take (RequestId);
      if (reply) {

         disconnect (
            reply, SIGNAL (finished ()),
            this, SLOT (_reply_finished ()));

         reply->abort ();
         reply->deleteLater ();

         emit reply_aborted (RequestId);
      }
   }
}


void
dmz::QtHttpClient::abort_all () {

   QList<UInt64> list = _replyMap.keys ();
   foreach (UInt64 id, list) { abort (id); }
}


//void
//dmz::QtHttpClient::update_username (const QString &Username, const QString &Password) {

//   _auth.setUser (Username);
//   _auth.setPassword (Password);
//}


//void
//dmz::QtHttpClient::_authenticate (QNetworkReply *reply, QAuthenticator *authenticator) {

//   if (reply && authenticator) {

//      const UInt64 RequestId = _get_request_id (reply);

//_log.error << "_____QtHttpClient authentication requested: " << RequestId << "_____"<< endl;

//       authenticator->setUser (_auth.user ());
//       authenticator->setPassword (_auth.password ());
//   }
//}


// QUrl
// dmz::QtHttpClient::_create_request_url (
//       const QUrl &BaseUrl,
//       const QStringList &UrlParts,
//       const QMap<QString, QString> &Params,
//       const QMap<QString, QString &DefaultParams) {
//
//    QUrl requestUrl (BaseUrl);
//
//    Int32 urlPartsAdded (0);
//    if (!UrlParts.isEmpty ()) {
//
//       urlPartsAdded = _add_parts_to_url (requestUrl, UrlParts);
//    }
//
//    Int32 paramsAdded (0);
//    if (!Params.isEmpty ()) {
//
//       paramsAdded = _add_params_to_url (requestUrl, Params);
//    }
//
//    if (!DefaultParams.isEmpty ()) {
//
//       paramsAdded += _add_params_to_url (requestUrl, DefaultParams);
//    }
//
//    return requestUrl;
// }
//
//
// dmz::Int32
// dmz::QtHttpClient::_add_parts_to_url (QUrl &url, const QStringList &Parts) {
//
//    Int32 result (0);
//    QString path = url.path ();
//
//    foreach (QString part, Parts) {
//
//       if (!path.endsWith (QChar ('/'))) { path.append ("/"); }
//       path.append (part);
//       result++;
//    }
//
//    url.setPath (path);
//    return result;
// }
//
//
// dmz::Int32
// dmz::QtHttpClient::_add_params_to_url (QUrl &url, const QMap<QString, QString> &Params) {
//
//    Int32 result (0);
//    QMapIterator<QString, QString> it (Params);
//    while (it.hasNext ()) {
//
//       url.addQueryItem (it.key (), it.value ());
//       result++;
//    }
//
//    return result;
// }


void
dmz::QtHttpClient::_reply_download_progress (qint64 bytesReceived, qint64 bytesTotal) {

   QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender ());
   if (reply) {

      const UInt64 RequestId (_get_request_id (reply));
      emit reply_download_progress (RequestId, reply, bytesReceived, bytesTotal);
   }
}


void
dmz::QtHttpClient::_reply_finished () {

   QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender ());
   if (reply) {

      // const Int32 StatusCode (reply->attribute (
      //    QNetworkRequest::HttpStatusCodeAttribute).toInt());

      const UInt64 RequestId (_get_request_id (reply));
      _replyMap.take (RequestId);

      emit reply_finished (RequestId, reply);

      // if (QNetworkReply::NoError == reply->error ()) {
      //
      //    emit reply_finished (RequestId, reply);
      // }
      // else if (QNetworkReply::OperationCanceledError == Error) {
      //
      // }
      // else {
      //
      // }

      reply->deleteLater ();
   }
}


void
dmz::QtHttpClient::_reply_error () {

//    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender ());
//    if (reply) {
//
//       const UInt64 RequestId = _get_request_id (reply);
//       const QString ErrorStr = reply->errorString ();
//       const QNetworkReply::NetworkError Error = reply->error ();
//
//       if (QNetworkReply::NoError == Error ||
//           QNetworkReply::OperationCanceledError == Error) {
//
//          // no error to report -ss
//       }
//       else {
//
// _log.warn << "QtHttpClient network error[" << RequestId << "]: " << (Int32)Error << " " << qPrintable (ErrorStr) << endl;
//
//          emit reply_error (RequestId, ErrorStr, Error);
//       }
//    }
}


void
dmz::QtHttpClient::_ssl_errors (QNetworkReply *reply, const QList<QSslError> &Errors) {

   if (reply) {

//      foreach (QSslError err, Errors) {

//         _log.warn << "SSL Error["<< (Int32)err.error () << "]: " << qPrintable (err.errorString ()) << endl;
//         if (err.error () == QSslError::) {

//         }
//      }

      reply->ignoreSslErrors ();
   }
}


QNetworkReply *
dmz::QtHttpClient::_request (
      const QString &Method,
      const QUrl &Url,
      const UInt64 RequestId,
      const QByteArray &Data) {

   QNetworkReply *reply (0);

   if (_manager) {

      QNetworkRequest request (Url);

      request.setRawHeader (LocalUserAgent, LocalUserAgentName);
      request.setRawHeader (LocalAccept, LocalApplicationJson);
      // request.setHeader (QNetworkRequest::ContentTypeHeader, LocalApplicationJson);

      request.setAttribute (LocalAttrId, RequestId);

      if (LocalGet == Method.toLower ()) {

         reply = _manager->get (request);
      }
      else if (LocalPut == Method.toLower ()) {

         reply = _manager->put (request, Data);
      }
      else if (LocalPost == Method.toLower ()) {

         request.setHeader (
            QNetworkRequest::ContentTypeHeader,
            LocalApplicationFormUrlEncoded);

         reply = _manager->post (request, Data);
      }
      else if (LocalDelete == Method.toLower ()) {

         reply = _manager->deleteResource (request);
      }
      else {

         _log.warn << "Unknown HTTP method requested: " << qPrintable (Method) << endl;
         _log.warn << "with url: " << qPrintable (Url.toString ()) << endl;
      }

      if (reply) {

         connect (
            reply, SIGNAL (downloadProgress (qint64, qint64)),
            this, SLOT (_reply_download_progress (qint64, qint64)));

         connect (reply, SIGNAL (finished ()), this, SLOT (_reply_finished ()));

         // connect (
         //    reply, SIGNAL (error (QNetworkReply::NetworkError)),
         //    this, SLOT (_reply_error ()));

         _replyMap.insert (RequestId, reply);
      }
   }

   return reply;
}


dmz::UInt64
dmz::QtHttpClient::_get_request_id (QNetworkReply *reply) const {

   UInt64 result (0);
   if (reply) {

      QNetworkRequest request = reply->request ();
      result = request.attribute (LocalAttrId).toULongLong ();
   }

   return result;
}
