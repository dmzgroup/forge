#include <dmzForgeQtWebService.h>
#include <dmzFoundationJSONUtil.h>
#include <dmzRuntimeConfig.h>
#include <dmzRuntimeConfigToTypesBase.h>
#include <dmzRuntimeLog.h>
#include <dmzTypesUUID.h>
#include <QtCore/QtCore>
#include <QtNetwork/QtNetwork>

#include <QtCore/QDebug>


struct dmz::ForgeQtWebService::State {

   Log *log;
   QUrl baseUrl;
   QNetworkAccessManager networkAccessManager;
   
   State ();
};


dmz::ForgeQtWebService::State::State () :
      log (0),
      baseUrl ("http://api.dmzforge.org"),
      networkAccessManager () {;}


dmz::ForgeQtWebService::ForgeQtWebService (QObject *parent, Log *log) :
      QObject (parent),
      _state (*(new State)) {

   _state.log = log;
}


dmz::ForgeQtWebService::~ForgeQtWebService () { 
   
   delete &_state;
}


void
dmz::ForgeQtWebService::set_host (const String &Host) {
   
   _state.baseUrl.setHost (Host.get_buffer ());
}


void
dmz::ForgeQtWebService::set_port (const Int32 Port) {

   _state.baseUrl.setPort (Port);
}


void
dmz::ForgeQtWebService::get_asset_list (const UInt32 Limit) {

   QUrl url (_state.baseUrl);
   url.setPath ("/assets");
   
   if (Limit) { url.addQueryItem ("limit", QString::number (Limit)); }
   
   QNetworkReply *reply = _get (url);
   if (reply) {
      
      connect (reply, SIGNAL (finished ()), SLOT (_get_asset_list_finished ()));
   }
}


void
dmz::ForgeQtWebService::get_asset (const UUID &Id) {

   QUrl url (_state.baseUrl);
   QString path ("/assets/%1");
   url.setPath (path.arg (Id.to_string (UUID::NotFormatted).get_buffer ()));
   
   QNetworkReply *reply = _get (url);
   if (reply) {
      
      connect (reply, SIGNAL (finished ()), SLOT (_get_asset_finished ()));
   }
}


QNetworkReply *
dmz::ForgeQtWebService::_get (const QUrl &Url) {

   QNetworkRequest request (Url);
   request.setRawHeader("User-Agent", "dmzForgeQtWebService 0.1");

   return _get (request);
}


QNetworkReply *
dmz::ForgeQtWebService::_get (const QNetworkRequest &Request) {

qDebug () << "_get: " << Request.url ().toString ();

   QNetworkReply *reply = _state.networkAccessManager.get (Request);
   return reply;
}


void
dmz::ForgeQtWebService::_get_asset_list_finished () {

   QNetworkReply *reply (qobject_cast<QNetworkReply *>(sender ()));
   if (reply) {

      if (reply->error() == QNetworkReply::NoError) {

         QString data (reply->readAll ());
         const String JSONData (qPrintable (data));
         Config cd ("global");
         
         if (json_string_to_config (JSONData, cd)) { _handle_asset_list (cd); }
         else { Q_EMIT error ("Error parsing json response."); }
      }
      else {

         String msg ("Network Error: ");
         msg << qPrintable (reply->errorString ());
         Q_EMIT error (msg);
      }
      
      reply->deleteLater ();
      reply = 0;
   }
}


void
dmz::ForgeQtWebService::_get_asset_finished () {

   QNetworkReply *reply (qobject_cast<QNetworkReply *>(sender ()));
   if (reply) {

      if (reply->error() == QNetworkReply::NoError) {

         QString data (reply->readAll ());
         const String JSONData (qPrintable (data));
         Config cd ("global");
         
         if (json_string_to_config (JSONData, cd)) { Q_EMIT asset_fetched (cd); }
         else { Q_EMIT error ("Error parsing json response."); }
      }
      else {

         String msg ("Network Error: ");
         msg << qPrintable (reply->errorString ());
         Q_EMIT error (msg);
      }
      
      reply->deleteLater ();
      reply = 0;
   }
}


void
dmz::ForgeQtWebService::_handle_asset_list (const Config &Response) {

   Config rowList;
   
   if (Response.lookup_all_config ("rows", rowList)) {
   
      StringContainer container;
      ConfigIterator it;
      Config asset;
      
      while (rowList.get_next_config (it, asset)) {
         
         String id = config_to_string ("id", asset);
         if (id) { container.append (id); }
      }
      
      Q_EMIT asset_list_fetched (container);
   }
}

