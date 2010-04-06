#include <dmzForgeConsts.h>
#include "dmzForgeWebServiceQt.h"
#include <QtCore/QtCore>
#include <QtNetwork/QtNetwork>

#include <QtCore/QDebug>


namespace dmz {
  
   const char ForgeApiEndpoint[] = "http://api.dmzforge.org";
   const char ForgeUserAgentName[] = "dmzForgeModuleQt";
};


dmz::ForgeWebServiceQt::ForgeWebServiceQt (Log *log, QObject *parent) :
      QObject (parent),
      _log (log),
      _nam (0),
      _baseUrl (ForgeApiEndpoint),
      _requestCounter (100) {

   _nam = new QNetworkAccessManager (this);
   
   connect (
      _nam, SIGNAL (finished (QNetworkReply *)),
      this, SIGNAL (finished (QNetworkReply *)));
}


dmz::ForgeWebServiceQt::~ForgeWebServiceQt () { 
   
}


void
dmz::ForgeWebServiceQt::set_host (const String &Host) {
   
   _baseUrl.setHost (Host.get_buffer ());
}


void
dmz::ForgeWebServiceQt::set_port (const Int32 Port) {

   _baseUrl.setPort (Port);
}


QNetworkReply *
dmz::ForgeWebServiceQt::search (const String &Value, const UInt32 Limit) {

   QUrl url (_baseUrl);
   url.setPath ("/assets/search");
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
   QString path ("/assets/%1");
   url.setPath (path.arg (AssetId.get_buffer ()));
   
   return _get (url, ForgeGetAssetName);
}


QNetworkReply *
dmz::ForgeWebServiceQt::put_asset (const String &AssetId, const String &JsonData) {

   QUrl url (_baseUrl);
   QString path ("/assets/%1");
   url.setPath (path.arg (AssetId.get_buffer ()));
   
   return _put (url, ForgePutAssetName, JsonData);
}


QNetworkReply *
dmz::ForgeWebServiceQt::delete_asset (const String &AssetId) {
   
   return 0;
}


QNetworkReply *
dmz::ForgeWebServiceQt::get_asset_media (const String &AssetId, const String &File) {
   
   return _get_attachment (AssetId, File, ForgeGetAssetMediaName);
}


QNetworkReply *
dmz::ForgeWebServiceQt::put_asset_media (
      const String &AssetId,
      const String &File,
      const String &MimeType) {

   return 0;
}


QNetworkReply *
dmz::ForgeWebServiceQt::get_asset_preview (const String &AssetId, const String &File) {
   
   return _get_attachment (AssetId, File, ForgeGetAssetPreviewName);
}


QNetworkReply *
dmz::ForgeWebServiceQt::put_asset_preview (const String &AssetId, const String &File) {

   return 0;
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
dmz::ForgeWebServiceQt::_get_attachment (
      const String &AssetId,
      const String &Attachemnt,
      const String &RequestType) {
   
   QUrl url (_baseUrl);
   QString path ("/assets/%1/%2");
   path = path.arg (AssetId.get_buffer ()).arg (Attachemnt.get_buffer ());
   url.setPath (path);
   
   return _get (url, RequestType);
}


