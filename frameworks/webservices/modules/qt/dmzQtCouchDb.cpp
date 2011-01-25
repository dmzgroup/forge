#include "dmzQtCouchDb.h"
#include "dmzQtHttpClient.h"
#include <QtCore/QUrl>


namespace {

};


dmz::QtCouchDb::QtCouchDb (
      const QUrl &Host,
      const QString &Database,
      const PluginInfo &Info,
      QObject *parent) :
      QObject (parent),
      _log (Info),
      _baseUrl (Host),
      _db (Database) {

   _baseUrl = 
   _client = new QtHttpClient (this);
   
   connect (
      _client, SIGNAL (reply_finished (const UInt64, QNetworkReply *)),
      this, SLOT (_reply_finished (const UInt64, QNetworkReply *)));
}


dmz::QtCouchDb::~QtCouchDb () {
   
   if (_client) {

      // abort_all ();
      _client->deleteLater ();
      _client = 0;
   }
}


dmz::UInt64
dmz::QtHttpClient::get_document () {

   UInt64 requestId (0);

   if (Url.isValid ()) {
      
      requestId = _requestCounter++;

      QNetworkReply *reply = _request (LocalGet, Url, requestId);
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


void
dmz::QtHttpClient::update_username (const QString &Username, const QString &Password) {

   _auth.setUser (Username);
   _auth.setPassword (Password);
}


void
dmz::QtHttpClient::_authenticate (QNetworkReply *reply, QAuthenticator *authenticator) {

   if (reply && authenticator) {
      
      const UInt64 RequestId = _get_request_id (reply);

_log.warn << "QtHttpClient authentication requested: " << RequestId << endl;
      
      authenticator->setUser (_auth.user ());
      authenticator->setPassword (_auth.password ());
   }
}


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
dmz::QtHttpClient::_reply_finished () {

   QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender ());
   if (reply) {

      const Int32 StatusCode =
         reply->attribute (QNetworkRequest::HttpStatusCodeAttribute).toInt();

      const UInt64 RequestId = _get_request_id (reply);

if (!RequestId) {

   _log.warn << "RequestId unknown: " << RequestId << endl;
}

      _replyMap.take (RequestId);
      emit reply_finished (RequestId, reply);
      reply->deleteLater ();
   }
}


void
dmz::QtHttpClient::_reply_error () {

   QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender ());
   if (reply) {
   
      const UInt64 RequestId = _get_request_id (reply);
      const QString ErrorStr = reply->errorString ();
      const QNetworkReply::NetworkError Error = reply->error ();

_log.warn << "QtHttpClient network error[" << RequestId << "]: " << qPrintable (ErrorStr) << endl;

      if (QNetworkReply::NoError == Error ||
          QNetworkReply::OperationCanceledError == Error) {
      
         // no error to report -ss
      }
      else {
         
         emit reply_error (RequestId, ErrorStr, Error);
      }
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

_log.warn << "_request: " << qPrintable (Url.toString ()) << endl;

      QNetworkRequest request (Url);

      request.setRawHeader (LocalUserAgent, LocalUserAgent);
      request.setRawHeader (LocalAccept, LocalApplicationJson);

      request.setAttribute (LocalAttrId, RequestId);

      if (LocalGet == Method.toLower ()) {

         reply = _manager->get (request);
      }
      else if (LocalPut == Method.toLower ()) {

         reply = _manager->put (request, Data);
      }
      else if (LocalDelete == Method.toLower ()) {

         reply = _manager->deleteResource (request);
      }
      else {

         _log.warn << "Unknown HTTP method requested: " << qPrintable (Method) << endl;
         _log.warn << "with url: " << qPrintable (Url.toString ()) << endl;
      }

      if (reply) {

         connect (reply, SIGNAL (finished ()), this, SLOT (_reply_finished ()));

         connect (
            reply, SIGNAL (error (QNetworkReply::NetworkError)),
            this, SLOT (_reply_error ()));
         
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

