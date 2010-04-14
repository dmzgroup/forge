#include "dmzForgeModuleQt.h"
#include <dmzForgeObserver.h>
#include <dmzFoundationJSONUtil.h>
#include <dmzFoundationSHA.h>
#include <dmzRuntimeConfig.h>
#include <dmzRuntimeConfigToTypesBase.h>
#include <dmzRuntimePluginFactoryLinkSymbol.h>
#include <dmzRuntimePluginInfo.h>
#include <dmzRuntimeLog.h>
#include <dmzSystem.h>
#include <dmzSystemFile.h>
#include <dmzSystemStreamString.h>
#include <dmzTypesHashTableStringTemplate.h>
#include <dmzTypesHashTableUInt64Template.h>
#include <dmzTypesString.h>
#include <dmzTypesStringContainer.h>
#include <dmzTypesUUID.h>
#include <QtCore/QtCore>
#include <QtNetwork/QtNetwork>


namespace {

   static const char ForgeApiEndpoint[] = "http://api.dmzforge.org:80";
   static const char ForgeUserAgentName[] = "dmzForgeModuleQt";
   
   static const dmz::String LocalJsonParseErrorMessage ("Error parsing json response.");

   static const dmz::String LocalId ("_id");
   static const dmz::String LocalRev ("_rev");
   static const dmz::String LocalName ("name");
   static const dmz::String LocalBrief ("brief");
   static const dmz::String LocalDetails ("details");
   static const dmz::String LocalKeywords ("keywords");
   static const dmz::String LocalThumbnails ("thumbnails");
   static const dmz::String LocalPreviews ("previews");
   static const dmz::String LocalImages ("images");
   static const dmz::String LocalType ("type");
   static const dmz::String LocalMedia ("media");
   static const dmz::String LocalCurrent ("current");
   static const dmz::String LocalMimeIVE ("model/x-ive");
   static const dmz::String LocalOriginalName ("original_name");
   static const dmz::String LocalValue ("value");
   
   enum LocalTypeEnum {
      LocalGetUuids = 201
   };
   
   static QNetworkRequest::Attribute LocalAttrType =
      (QNetworkRequest::Attribute) (QNetworkRequest::User + 1);
      
   static QNetworkRequest::Attribute LocalAttrId =
      (QNetworkRequest::Attribute) (QNetworkRequest::User + 2);
   
   struct UploadStruct {
      
      dmz::UInt64 requestId;
      dmz::String assetId;
      dmz::String file;
   };
};


struct dmz::ForgeModuleQt::AssetStruct {

   const String Id;
   String revision;
   ForgeAssetTypeEnum type;
   String name;
   String brief;
   String details;
   StringContainer keywords;
   StringContainer media;
   StringContainer previews;
   
   AssetStruct (const String &AssetId) : Id (AssetId) {;}
};


// struct dmz::ForgeModuleQt::UploadStruct {
// 
// 
// };


struct dmz::ForgeModuleQt::State {

   Log log;
   QNetworkAccessManager *networkAccessManager;
   StringContainer uuids;
   UInt64 requestCounter;
   HashTableUInt64Template<ForgeObserver> obsTable;
   HashTableStringTemplate<AssetStruct> assetTable;
   QQueue<UploadStruct *> uploadQueue;
   Boolean uploading;
   QUrl baseUrl;
   QString db;
   
   State (const PluginInfo &Info);
   ~State ();
};


dmz::ForgeModuleQt::State::State (const PluginInfo &Info) :
      log (Info),
      networkAccessManager (0),
      requestCounter (1000),
      uploading (False),
      baseUrl (ForgeApiEndpoint) {;}


dmz::ForgeModuleQt::State::~State () {

   if (networkAccessManager) { networkAccessManager->deleteLater (); }
   
   obsTable.clear ();
   assetTable.empty ();
}


dmz::ForgeModuleQt::ForgeModuleQt (const PluginInfo &Info, Config &local) :
      QObject (0),
      Plugin (Info),
      ForgeModule (Info),
      _state (*(new State (Info))) {

   _state.networkAccessManager = new QNetworkAccessManager (this);

   connect (
      _state.networkAccessManager, SIGNAL (finished (QNetworkReply *)),
      this, SLOT (_handle_reply (QNetworkReply *)));

   _init (local);
}


dmz::ForgeModuleQt::~ForgeModuleQt () {

   delete &_state;
}


// Plugin Interface
void
dmz::ForgeModuleQt::update_plugin_state (
      const PluginStateEnum State,
      const UInt32 Level) {

   if (State == PluginStateInit) {

      _get_uuid (); // this will prefetch some uuids from the server -ss
   }
   else if (State == PluginStateStart) {

   }
   else if (State == PluginStateStop) {

   }
   else if (State == PluginStateShutdown) {

   }
}


void
dmz::ForgeModuleQt::discover_plugin (
      const PluginDiscoverEnum Mode,
      const Plugin *PluginPtr) {

   if (Mode == PluginDiscoverAdd) {

   }
   else if (Mode == PluginDiscoverRemove) {

   }
}


// ForgeModule Interface

dmz::Boolean
dmz::ForgeModuleQt::is_saved (const String &AssetId) {

   Boolean retVal (False);
   
   return retVal;
}


dmz::String
dmz::ForgeModuleQt::create_asset (const String &AssetId) {

   String retVal;
   
   AssetStruct *as = _state.assetTable.lookup (AssetId);
   
   if (!as) {
   
      as = new AssetStruct (AssetId ? AssetId : _get_uuid ());
      
      if (!_state.assetTable.store (as->Id, as)) {
         
         delete as;
         as = 0;
      }
   }
   
   if (as) { retVal = as->Id; }
   
   return retVal;
}


dmz::ForgeAssetTypeEnum
dmz::ForgeModuleQt::lookup_asset_type (const String &AssetId) {

   ForgeAssetTypeEnum retVal (ForgeAssetUnknown);
   AssetStruct *asset = _state.assetTable.lookup (AssetId);
   if (asset) { retVal = asset->type; }
   return retVal;
}


dmz::Boolean
dmz::ForgeModuleQt::store_name (const String &AssetId, const String &Value) {
   
   Boolean retVal (False);
   AssetStruct *asset = _state.assetTable.lookup (AssetId);
   if (asset) { asset->name = Value; retVal = True; }
   return retVal;
}


dmz::Boolean
dmz::ForgeModuleQt::lookup_name (const String &AssetId, String &value) {
   
   Boolean retVal (False);
   AssetStruct *asset = _state.assetTable.lookup (AssetId);
   if (asset) { value = asset->name; retVal = True; }
   return retVal;
}


dmz::Boolean
dmz::ForgeModuleQt::store_brief (const String &AssetId, const String &Value) {
   
   Boolean retVal (False);
   AssetStruct *asset = _state.assetTable.lookup (AssetId);
   if (asset) { asset->brief = Value; retVal = True; }
   return retVal;
}


dmz::Boolean
dmz::ForgeModuleQt::lookup_brief (const String &AssetId, String &value) {
   
   Boolean retVal (False);
   AssetStruct *asset = _state.assetTable.lookup (AssetId);
   if (asset) { value = asset->brief; retVal = True; }
   return retVal;
}


dmz::Boolean
dmz::ForgeModuleQt::store_details (const String &AssetId, const String &Value) {
   
   Boolean retVal (False);
   AssetStruct *asset = _state.assetTable.lookup (AssetId);
   if (asset) { asset->details = Value; retVal = True; }
   return retVal;
}


dmz::Boolean
dmz::ForgeModuleQt::lookup_details (const String &AssetId, String &value) {
   
   Boolean retVal (False);
   AssetStruct *asset = _state.assetTable.lookup (AssetId);
   if (asset) { value = asset->details; retVal = True; }
   return retVal;
}


dmz::Boolean
dmz::ForgeModuleQt::store_keywords (const String &AssetId, const StringContainer &Value) {

   Boolean retVal (False);
   AssetStruct *asset = _state.assetTable.lookup (AssetId);
   if (asset) { asset->keywords = Value; retVal = True; }
   return retVal;
}


dmz::Boolean
dmz::ForgeModuleQt::lookup_keywords (const String &AssetId, StringContainer &value) {
   
   Boolean retVal (False);
   AssetStruct *asset = _state.assetTable.lookup (AssetId);
   if (asset) { value = asset->keywords; retVal = True; }
   return retVal;
}


dmz::Boolean
dmz::ForgeModuleQt::lookup_asset_media (const String &AssetId, StringContainer &value) {
   
   Boolean retVal (False);
   AssetStruct *asset = _state.assetTable.lookup (AssetId);
   if (asset) { value = asset->media; retVal = True; }
   return retVal;
}


dmz::Boolean
dmz::ForgeModuleQt::lookup_previews (const String &AssetId, StringContainer &value) {
      
   Boolean retVal (False);
   AssetStruct *asset = _state.assetTable.lookup (AssetId);
   if (asset) { value = asset->previews; retVal = True; }
   return retVal;
}


// methods that communicate with api.dmzforge.org

dmz::UInt64
dmz::ForgeModuleQt::search (
      const String &Value,
      ForgeObserver *observer,
      const UInt32 Limit) {

   UInt64 requestId (0);
   
   if (_state.networkAccessManager && observer) {
      
      requestId = _state.requestCounter++;
      _state.obsTable.store (requestId, observer);
      
      QUrl url (_state.baseUrl);
      QString path ("/%1/search");
      url.setPath (path.arg (_state.db));
      url.addQueryItem ("q", Value.get_buffer ());

      if (Limit) { url.addQueryItem ("limit", QString::number (Limit)); }

      QNetworkRequest request (url);
      request.setAttribute (LocalAttrId, requestId);
      request.setAttribute (LocalAttrType, ForgeSearch);

      QNetworkReply *reply = _state.networkAccessManager->get (request);
   }
   
   return requestId;
}


dmz::UInt64
dmz::ForgeModuleQt::get_asset (const String &AssetId, ForgeObserver *observer) {

   UInt64 requestId (0);
   
   if (_state.networkAccessManager && observer) {
      
      requestId = _state.requestCounter++;
      _state.obsTable.store (requestId, observer);
      
      QUrl url (_state.baseUrl);
      QString path ("/%1/%2");
      url.setPath (path.arg (_state.db).arg (AssetId.get_buffer ()));

      QNetworkRequest request (url);
      request.setAttribute (LocalAttrId, requestId);
      request.setAttribute (LocalAttrType, ForgeGetAsset);

      QNetworkReply *reply = _state.networkAccessManager->get (request);
   }
   
   return requestId;
}


dmz::UInt64
dmz::ForgeModuleQt::put_asset (const String &AssetId, ForgeObserver *observer) {

   UInt64 requestId (0);
   
   AssetStruct *as = _state.assetTable.lookup (AssetId);
   
   if (_state.networkAccessManager && observer && as) {

      String assetJson;
      
      if (_asset_to_json (AssetId, assetJson)) {
         
         requestId = _state.requestCounter++;

         _state.obsTable.store (requestId, observer);
         
         QUrl url (_state.baseUrl);
         QString path ("/%1/%2");
         url.setPath (path.arg (_state.db).arg (AssetId.get_buffer ()));

         QByteArray byteArray (assetJson.get_buffer ());
         
         QNetworkRequest request (url);
         // request.setRawHeader("User-Agent", ForgeUserAgentName);
         request.setAttribute (LocalAttrId, requestId);
         request.setAttribute (LocalAttrType, ForgePutAsset);

         QNetworkReply *reply = _state.networkAccessManager->put (request, byteArray);
      }
   }
   
   return requestId;
}


dmz::UInt64
dmz::ForgeModuleQt::delete_asset (const String &AssetId, ForgeObserver *observer) {

   UInt64 retVal (0);
   
   return retVal;
}


dmz::UInt64
dmz::ForgeModuleQt::get_asset_media (
      const String &AssetId,
      const String &File,
      ForgeObserver *observer) {

   UInt64 retVal (0);

   return retVal;
}


dmz::UInt64
dmz::ForgeModuleQt::put_asset_media (
      const String &AssetId,
      const String &File,
      ForgeObserver *observer,
      const String &MimeType) {

   UInt64 retVal (0);

   return retVal;
}


dmz::UInt64
dmz::ForgeModuleQt::get_asset_preview (
      const String &AssetId,
      const String &File,
      ForgeObserver *observer) {

   UInt64 retVal (0);

   return retVal;
}


dmz::UInt64
dmz::ForgeModuleQt::put_asset_preview (
      const String &AssetId,
      const String &Preview,
      ForgeObserver *observer) {

   UInt64 requestId (0);

   AssetStruct *as = _state.assetTable.lookup (AssetId);
   
   if (AssetId && observer) {

      requestId = _state.requestCounter++;

      _state.obsTable.store (requestId, observer);

      UploadStruct *us = new UploadStruct;
      us->requestId = requestId;
      us->assetId = AssetId;
      us->file = Preview;
      
      _state.uploadQueue.enqueue (us);

      // if (!_state.uploading) { _start_next_upload (); }
   }
   
   return requestId;
}


void
dmz::ForgeModuleQt::_handle_reply (QNetworkReply *reply) {
   
   if (reply) {

      const Int32 StatusCode =
         reply->attribute (QNetworkRequest::HttpStatusCodeAttribute).toInt();

      QNetworkRequest request = reply->request ();
      
      const Int32 Type = request.attribute (LocalAttrType).toInt ();
      const UInt64 Id = request.attribute (LocalAttrId).toULongLong ();

      QString data (reply->readAll ());
      const String JsonData (qPrintable (data));
      
_state.log.warn << "_handle_reply[" << StatusCode << "]: " << Type << endl;
_state.log.warn << "json: " << JsonData << endl;

      switch (Type) {

         case ForgeSearch:
            _handle_search (Id, JsonData);
            break;
         
         case ForgeGetAsset:
            _handle_get_asset (Id, JsonData);
            break;
            
         case ForgePutAsset:
            _handle_put_asset (Id, JsonData);
            break;
         
         case ForgeDeleteAsset:
         break;
         
         case ForgeGetAssetMedia:
         break;
         
         case ForgePutAssetMedia:
         break;
         
         case ForgeGetAssetPreview:
         break;
         
         case ForgePutAssetPreview:
         break;
         
         case LocalGetUuids:
            _handle_get_uuids (Id, JsonData);
            break;
         
         default:
            String msg ("Unknown request type: ");
            msg << Type;

            _handle_error (Id, (ForgeTypeEnum)Type, msg);
      }
      
      // else {
      //    
      //    QString data (reply->readAll ());
      //    _state.log.warn << "json: " << qPrintable (data) << endl;
      // 
      //    String msg ("Network Error: ");
      //    msg << qPrintable (reply->errorString ());
      //    
      //    _handle_error (RequestId, qPrintable (RequestType), msg);
      // }
      
      _state.obsTable.remove (Id);
      
      reply->deleteLater ();
   }
}


void
dmz::ForgeModuleQt::_handle_search (const UInt64 RequestId, const String &JsonData) {

   Config global ("global");
   
   if (json_string_to_config (JsonData, global)) { 
   
      StringContainer container;
      Config list;

      if (global.lookup_all_config ("rows", list)) {

         ConfigIterator it;
         Config cd;

         while (list.get_next_config (it, cd)) {

            String assetId = config_to_string ("id", cd);
            if (assetId) { container.append (assetId); }
         }
      }
      
      _handle_reply (RequestId, ForgeSearch, container);
   }
   else {
      
      _handle_error (RequestId, ForgeSearch, LocalJsonParseErrorMessage);
   }
}


void
dmz::ForgeModuleQt::_handle_get_asset (const UInt64 RequestId, const String &JsonData) {

   Config global ("global");
   
   if (json_string_to_config (JsonData, global)) { 
   
      const String AssetId (config_to_string (LocalId, global));

_state.log.error << "_handle_get_asset: " << AssetId << endl;
_state.log.error << "json: " << JsonData << endl;

      _config_to_asset (AssetId, global);

      StringContainer container;
      container.append (JsonData);
      
      _handle_reply (RequestId, ForgeGetAsset, container);
   }
   else {
      
      _handle_error (RequestId, ForgeGetAsset, LocalJsonParseErrorMessage);
   }
}


void
dmz::ForgeModuleQt::_handle_put_asset (const UInt64 RequestId, const String &JsonData) {

   Config global ("global");
   
   if (json_string_to_config (JsonData, global)) { 
   
      const String AssetId (config_to_string ("id", global));
      const String Revision (config_to_string ("rev", global));
      _store_revision (AssetId, Revision);
      
      StringContainer container;
      container.append (JsonData);
      
      _handle_reply (RequestId, ForgePutAsset, container);
   }
   else {
      
      _handle_error (RequestId, ForgePutAsset, LocalJsonParseErrorMessage);
   }
}


void
dmz::ForgeModuleQt::_handle_get_uuids (const UInt64 RequestId, const String &JsonData) {

   Config global ("global");
   
   if (json_string_to_config (JsonData, global)) { 

      Config list;
   
      if (global.lookup_all_config ("uuids", list)) {
   
         ConfigIterator it;
         Config cd;
   
         while (list.get_next_config (it, cd)) {
   
            String id = config_to_string ("value", cd);
            if (id) { _state.uuids.append (id); }
         }
         
_state.log.info << "uuids count: " << _state.uuids.get_count () << endl;
      }
   }
}


void
dmz::ForgeModuleQt::_handle_reply (
      const UInt64 RequestId,
      const ForgeTypeEnum &RequestType,
      StringContainer &Container) {

   ForgeObserver *observer = _state.obsTable.lookup (RequestId);
   if (observer) {
      
      observer->handle_reply (RequestId, RequestType, False, Container);
   }
}


void
dmz::ForgeModuleQt::_handle_error (
      const UInt64 RequestId,
      const ForgeTypeEnum &RequestType,
      const String &Message) {

   ForgeObserver *observer = _state.obsTable.lookup (RequestId);
   if (observer) {
      
      StringContainer container;
      container.append (Message);
      
      observer->handle_reply (RequestId, RequestType, True, container);
   }
}


dmz::Boolean
dmz::ForgeModuleQt::_store_revision (const String &AssetId, const String &Value) {
   
_state.log.error << "store revision: " << Value << " for asset: " << AssetId << endl;
   Boolean retVal (False);
   AssetStruct *asset = _state.assetTable.lookup (AssetId);
   if (asset && Value) { asset->revision = Value; retVal = True; }
   return retVal;
}


dmz::Boolean
dmz::ForgeModuleQt::_lookup_revision (const String &AssetId, String &value) {
   
   Boolean retVal (False);
   AssetStruct *asset = _state.assetTable.lookup (AssetId);
   if (asset) { value = asset->revision; retVal = True; }
   return retVal;
}


void
dmz::ForgeModuleQt::_start_next_upload () {

//    if (!_state.uploading && !_state.uploadQueue.isEmpty ()) {
//    
// _state.log.warn << "_start_next_upload: " << (Int32)_state.uploadQueue.count () << endl;
//       
//       UploadStruct *us = _state.uploadQueue.dequeue ();
// 
//       if (us && _state.ws) {
// 
// _state.log.debug << "us: requestId: " << us->requestId << endl;
// _state.log.debug << "us: assetId: " << us->assetId << endl;
// _state.log.debug << "us: file: " << us->file << endl;
// 
//          String revision;
// 
//          if (_lookup_revision (us->assetId, revision)) {
// 
//             QNetworkReply *reply =
//                _state.ws->put_asset_preview (us->assetId, revision, us->file);
// 
//             if (reply) {
// 
//                _state.uploading = True;
// 
//                // connect (reply, SIGNAL (finished ()), this, SLOT (_start_next_upload ()));
// 
//                reply->setProperty ("requestId", us->requestId);
//             }
//          } 
//       }
//       else {
//          
//          _state.uploading = False;
//       }
//    }
}


dmz::Boolean
dmz::ForgeModuleQt::_asset_to_json (const String &AssetId, String &assetJson) {

   Boolean retVal (False);
   
   Config config ("global");
      
   if (_asset_to_config (AssetId, config)) {
         
      String data;
      StreamString out (data);

      retVal = format_config_to_json (config, out, ConfigStripGlobal, &_state.log);

      if (retVal) { assetJson = data; }
   }
   
   return retVal;
}


dmz::Boolean
dmz::ForgeModuleQt::_asset_to_config (const String &AssetId, Config &assetConfig) {

   Boolean retVal (False);
   
   AssetStruct *as = _state.assetTable.lookup (AssetId);
   if (as) {
      
      assetConfig.store_attribute (LocalId, as->Id);

      if (as->revision) { assetConfig.store_attribute (LocalRev, as->revision); }

      assetConfig.store_attribute (LocalType, "asset");
      assetConfig.store_attribute (LocalName, as->name);
      assetConfig.store_attribute (LocalBrief, as->brief);
      assetConfig.store_attribute (LocalDetails, as->details);
      
      String keyword;
      Boolean first = True;
      
      while (as->keywords.get_next (keyword)) {
      
         Config data (LocalKeywords);
         data.store_attribute (LocalValue, keyword);
         
         if (first) { assetConfig.overwrite_config (data); first = False; }
         else { assetConfig.add_config (data); }
      }
      
      retVal = True;
   }
   
   return retVal;
}


dmz::Boolean
dmz::ForgeModuleQt::_config_to_asset (const String &AssetId, const Config &AssetConfig) {

   Boolean retVal (False);
   
   AssetStruct *as = _state.assetTable.lookup (AssetId);
   if (!as && AssetId) {
      
      create_asset (AssetId);
      as = _state.assetTable.lookup (AssetId);
   }
   
   if (as) {
      
      _store_revision (AssetId, config_to_string (LocalRev, AssetConfig));
      
      store_name (AssetId, config_to_string (LocalName, AssetConfig));
      store_brief (AssetId, config_to_string (LocalBrief, AssetConfig));
      store_details (AssetId, config_to_string (LocalDetails, AssetConfig));
      
      StringContainer keywords;
      Config keywordList;
      
      if (AssetConfig.lookup_all_config (LocalKeywords, keywordList)) {
         
         ConfigIterator it;
         Config word;
         
         while (keywordList.get_next_config (it, word)) {
            
            keywords.append (config_to_string (LocalValue, word));
         }
      }
      
      store_keywords (AssetId, keywords);
      
      retVal = True;
   }
   
   return retVal;
}


dmz::String
dmz::ForgeModuleQt::_get_uuid () {
   
   String retVal;
   
   if (_state.uuids.get_count () < 5) {
      
      if (_state.networkAccessManager) {
         
         UInt64 requestId = _state.requestCounter++;
         
         QUrl url (_state.baseUrl);
         url.setPath ("/_uuids");
         url.addQueryItem ("count", QString::number (100));

         QNetworkRequest request (url);
         request.setAttribute (LocalAttrId, requestId);
         request.setAttribute (LocalAttrType, LocalGetUuids);

         QNetworkReply *reply = _state.networkAccessManager->get (request);
      }
      
      UUID id;
      create_uuid (id);
      retVal = id.to_string (UUID::NotFormatted);
   }
   else {
      
      _state.uuids.get_first (retVal);
      _state.uuids.remove (retVal);
   }
   
   
   return retVal;
}


void
dmz::ForgeModuleQt::_init (Config &local) {

   setObjectName (get_plugin_name ().get_buffer ());
   
   Config proxy;
   if (local.lookup_config ("webservice.proxy", proxy)) {
      
      String host = config_to_string ("webservice.host", proxy, "localhost");
      Int32 port = config_to_int32 ("webservice.port", proxy, 8888);
      
      QNetworkProxy proxy;
      proxy.setType (QNetworkProxy::HttpProxy);
      proxy.setHostName (host.get_buffer ());
      proxy.setPort (port);
      
      QNetworkProxy::setApplicationProxy(proxy);
      
      _state.log.info << "Using proxy: " << host << ":" << port << endl;
   }

   String host =  config_to_string ("host", local, qPrintable (_state.baseUrl.host ()));
   _state.baseUrl.setHost (host.get_buffer ());
      
   _state.baseUrl.setPort (config_to_int32 ("port", local, _state.baseUrl.port ()));
   
   _state.log.info << "Using API endpoint: "
                   << qPrintable (_state.baseUrl.toString ()) << endl;
   
   _state.db = config_to_string ("webservice.db", local, "assets").get_buffer ();
}


extern "C" {

DMZ_PLUGIN_FACTORY_LINK_SYMBOL dmz::Plugin *
create_dmzForgeModuleQt (
      const dmz::PluginInfo &Info,
      dmz::Config &local,
      dmz::Config &global) {

   return new dmz::ForgeModuleQt (Info, local);
}

};
