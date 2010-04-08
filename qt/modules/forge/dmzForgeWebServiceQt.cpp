#include <dmzForgeConsts.h>
#include "dmzForgeWebServiceQt.h"
#include <dmzRuntimeConfigToTypesBase.h>
#include <QtCore/QtCore>
#include <QtNetwork/QtNetwork>

#include <QtCore/QDebug>


namespace dmz {
  
   const char ForgeApiEndpoint[] = "http://api.dmzforge.org:80";
   const char ForgeUserAgentName[] = "dmzForgeModuleQt";
};


dmz::ForgeWebServiceQt::ForgeWebServiceQt (Config &local, Log *log, QObject *parent) :
      QObject (parent),
      _log (log),
      _nam (0),
      _baseUrl (ForgeApiEndpoint),
      _requestCounter (100) {

   _init (local);
}


dmz::ForgeWebServiceQt::~ForgeWebServiceQt () { 
   
}


void
dmz::ForgeWebServiceQt::set_host (const String &Host) {
   
   _baseUrl.setHost (Host.get_buffer ());
}


dmz::String
dmz::ForgeWebServiceQt::get_host () const {
   
   return qPrintable (_baseUrl.host ());
}


void
dmz::ForgeWebServiceQt::set_port (const Int32 Port) {

   _baseUrl.setPort (Port);
}


dmz::Int32
dmz::ForgeWebServiceQt::get_port () const {

   return _baseUrl.port ();
}


QNetworkReply *
dmz::ForgeWebServiceQt::search (const String &Value, const UInt32 Limit) {

   QUrl url (_baseUrl);
   QString path ("/%1/search");
   url.setPath (path.arg (_db));
   url.addQueryItem ("q", Value.get_buffer ());
   
   if (Limit) { url.addQueryItem ("limit", QString::number (Limit)); }
   
   return _get (url, ForgeSearchName);
}


QNetworkReply *
dmz::ForgeWebServiceQt::get_uuids (const UInt32 Count) {

   QUrl url (_baseUrl);
   url.setPath ("/uuids");
   
   if (Count) { url.addQueryItem ("count", QString::number (Count)); }
   
   return _get (url, "get_uuids");
}


QNetworkReply *
dmz::ForgeWebServiceQt::get_asset (const String &AssetId) {

   QUrl url (_baseUrl);
   QString path ("/%1/%2");
   url.setPath (path.arg (_db).arg (AssetId.get_buffer ()));
   
   return _get (url, ForgeGetAssetName);
}


QNetworkReply *
dmz::ForgeWebServiceQt::put_asset (const String &AssetId, const String &JsonData) {

   QUrl url (_baseUrl);
   QString path ("/%1/%2");
   url.setPath (path.arg (_db).arg (AssetId.get_buffer ()));
   
   return _put (url, ForgePutAssetName, JsonData);
}


QNetworkReply *
dmz::ForgeWebServiceQt::delete_asset (const String &AssetId, const String &Revision) {
   
   QUrl url (_baseUrl);
   QString path ("/%1/%2");
   
   url.setPath (path.arg (_db).arg (AssetId.get_buffer ()));
   
   url.addQueryItem ("rev", Revision.get_buffer ());
   
   return _delete (url, ForgeDeleteAssetName);
}


QNetworkReply *
dmz::ForgeWebServiceQt::get_asset_media (const String &AssetId, const String &File) {
   
   return _get_attachment (AssetId, File, ForgeGetAssetMediaName);
}


QNetworkReply *
dmz::ForgeWebServiceQt::put_asset_media (
      const String &AssetId,
      const String &File,
      const String &Revision,
      const String &MimeType) {

   return 0;
}


QNetworkReply *
dmz::ForgeWebServiceQt::get_asset_preview (const String &AssetId, const String &File) {
   
   return _get_attachment (AssetId, File, ForgeGetAssetPreviewName);
}


QNetworkReply *
dmz::ForgeWebServiceQt::put_asset_preview (
      const String &AssetId,
      const String &Revision,
      const String &File) {

   QFileInfo fi (File.get_buffer ());

//   QUrl url (_baseUrl);
   QUrl url ("http://api.dmzforge.org:5984");
   QString path ("/%1/%2/%3");
   url.setPath (path.arg (_db).arg (AssetId.get_buffer ()).arg (fi.fileName ()));
   
   url.addQueryItem ("rev", Revision.get_buffer ());
   
   return _put_file (url, ForgePutAssetPreviewName, File);
}


QNetworkReply *
dmz::ForgeWebServiceQt::_get (const QUrl &Url, const String &RequestType) {

   QNetworkRequest request (Url);
   request.setRawHeader("User-Agent", ForgeUserAgentName);

qDebug () << "_get: " << request.url ().toString ();

   QNetworkReply *reply = _nam->get (request);
   if (reply) {
      
      QVariant type (RequestType.get_buffer ());
      reply->setProperty ("requestType", type);
      
      QVariant id (_requestCounter++);
      reply->setProperty ("requestId", id);
      
      connect (reply, SIGNAL (finished ()), reply, SLOT (deleteLater ()));
   }
   
   return reply;
}


QNetworkReply *
dmz::ForgeWebServiceQt::_put (
      const QUrl &Url,
      const String &RequestType,
      const String &Data) {

   QNetworkRequest request (Url);
   request.setRawHeader("User-Agent", ForgeUserAgentName);

qDebug () << "_put: " << request.url ().toString ();

   QByteArray byteArray (Data.get_buffer ());

   QNetworkReply *reply = _nam->put (request, byteArray);
   if (reply) {
      
      QVariant type (RequestType.get_buffer ());
      reply->setProperty ("requestType", type);
      
      QVariant id (_requestCounter++);
      reply->setProperty ("requestId", id);
      
      connect (reply, SIGNAL (finished ()), reply, SLOT (deleteLater ()));
   }
   
   return reply;
}


QNetworkReply *
dmz::ForgeWebServiceQt::_put_file (
      const QUrl &Url,
      const String &RequestType,
      const String &File) {

   QFileInfo fi (File.get_buffer ());

   QNetworkRequest request (Url);
   request.setRawHeader("User-Agent", ForgeUserAgentName);
   request.setRawHeader("Content-Type", "image/jpeg");

   QNetworkReply *reply (0);
   QFile *file = new QFile (fi.absoluteFilePath ());
   
   if (file->open (QIODevice::ReadOnly)) {
      
      qDebug () << "_put_file: " << request.url ().toString ();

      reply = _nam->put (request, file);
      if (reply) {

         QVariant type (RequestType.get_buffer ());
         reply->setProperty ("requestType", type);

         QVariant id (_requestCounter++);
         reply->setProperty ("requestId", id);

         connect (reply, SIGNAL (finished ()), reply, SLOT (deleteLater ()));

         connect (
            reply, SIGNAL (uploadProgress (qint64, qint64)),
            this, SLOT (_upload_progress (qint64, qint64)));
      }
   }
   else {
      
      if (_log) { _log->warn << "Unable to open file for uploading: " << File << endl; }
   }
   
   return reply;
}


QNetworkReply *
dmz::ForgeWebServiceQt::_delete (const QUrl &Url, const String &RequestType) {

   QNetworkRequest request (Url);
   request.setRawHeader("User-Agent", ForgeUserAgentName);

qWarning () << "deleteResource is only available in Qt 4.6";

qDebug () << "_delete: " << request.url ().toString ();

   // deleteResource is only available in Qt 4.6
   
   // QNetworkReply *reply = _nam->deleteResource (request);
   // if (reply) {
   //    
   //    QVariant type (RequestType.get_buffer ());
   //    reply->setProperty ("requestType", type);
   //    
   //    QVariant id (_requestCounter++);
   //    reply->setProperty ("requestId", id);
   //    
   //    connect (reply, SIGNAL (finished ()), reply, SLOT (deleteLater ()));
   // }
   // 
   // return reply;
   
   return 0;
}


QNetworkReply *
dmz::ForgeWebServiceQt::_get_attachment (
      const String &AssetId,
      const String &Attachemnt,
      const String &RequestType) {
   
   QUrl url (_baseUrl);
   QString path ("/%1/%2/%3");
   
   path = path.arg (_db)
              .arg (AssetId.get_buffer ())
              .arg (Attachemnt.get_buffer ());
      
   url.setPath (path);
   
   return _get (url, RequestType);
}


void
dmz::ForgeWebServiceQt::_upload_progress (qint64 bytesSent, qint64 bytesTotal) {

   if (_log) {
      
      _log->warn << "_upload_progress: " << bytesSent << " of " << bytesTotal << endl;
   }
}


void
dmz::ForgeWebServiceQt::_init (Config &local) {

   Config proxy;
   if (local.lookup_config ("proxy", proxy)) {
      
      String host = config_to_string ("host", proxy, "localhost");
      Int32 port = config_to_int32 ("port", proxy, 8888);
      
      QNetworkProxy proxy;
      proxy.setType (QNetworkProxy::HttpProxy);
      proxy.setHostName (host.get_buffer ());
      proxy.setPort (port);
      
      QNetworkProxy::setApplicationProxy(proxy);
      
      if (_log) { _log->info << "Using proxy: " << host << ":" << port << endl; }
   }

   _nam = new QNetworkAccessManager (this);
   
   connect (
      _nam, SIGNAL (finished (QNetworkReply *)),
      this, SIGNAL (finished (QNetworkReply *)));
   
   set_host (config_to_string ("host", local, get_host ()));
   set_port (config_to_int32 ("port", local, get_port ()));
   
   if (_log) {
      
      _log->info << "Using API endpoint: " << qPrintable (_baseUrl.toString ()) << endl;
   }
   
   _db = config_to_string ("db", local, "assets").get_buffer ();
}
