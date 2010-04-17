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
   
   static char LocalDefaultMimeType[] = "application/octet-stream";
   static QHash<QString, QByteArray> localMimeTypes;
   
   enum LocalTypeEnum {
      LocalGetUuids = 201,
      LocalPutAsset = 203
   };
   
   static QNetworkRequest::Attribute LocalAttrType =
      (QNetworkRequest::Attribute) (QNetworkRequest::User + 1);
      
   static QNetworkRequest::Attribute LocalAttrId =
      (QNetworkRequest::Attribute) (QNetworkRequest::User + 2);

   // static QNetworkRequest::Attribute LocalAttrFilesCount =
   //    (QNetworkRequest::Attribute) (QNetworkRequest::User + 3);
   
   struct UploadStruct {
      
      dmz::UInt64 requestId;
      dmz::ForgeTypeEnum type;
      dmz::String assetId;
      dmz::StringContainer files;
      dmz::String mimeType;
      
      UploadStruct () : requestId (0), type (dmz::ForgeTypeUnknown) {;}
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
   UploadStruct *upload;
   QNetworkReply *uploadReply;
   QFile *uploadFile;
   
   State (const PluginInfo &Info);
   ~State ();
};


void
local_init_mime_types () {

   // images
   localMimeTypes["png"]      = "image/png";
   localMimeTypes["jpg"]      = "image/jpeg";
   localMimeTypes["tiff"]     = "image/tiff";
   localMimeTypes["svg"]      = "image/svg+xml";
   localMimeTypes["collads"]  = "model/x-collada+xml";
   localMimeTypes["ive"]      = "model/x-ive";
   localMimeTypes["wav"]      = "audio/x-wav";
   localMimeTypes["xml"]      = "application/x-dmz+xml";
   localMimeTypes["json"]     = "application/x-dmz+json";
}


dmz::ForgeModuleQt::State::State (const PluginInfo &Info) :
      log (Info),
      networkAccessManager (0),
      requestCounter (1000),
      uploading (False),
      baseUrl (ForgeApiEndpoint),
      upload (0),
      uploadReply (0),
      uploadFile (0) {;}


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

   local_init_mime_types ();
         
   _state.networkAccessManager = new QNetworkAccessManager (this);

   connect (
      _state.networkAccessManager, SIGNAL (finished (QNetworkReply *)),
      this, SLOT (_reply_finished (QNetworkReply *)));

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
      request.setRawHeader ("User-Agent", ForgeUserAgentName);
      request.setRawHeader ("Accept", "application/json");
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
      request.setRawHeader ("User-Agent", ForgeUserAgentName);
      request.setRawHeader ("Accept", "application/json");
      request.setAttribute (LocalAttrId, requestId);
      request.setAttribute (LocalAttrType, ForgeGetAsset);

      QNetworkReply *reply = _state.networkAccessManager->get (request);
   }
   
   return requestId;
}


dmz::UInt64
dmz::ForgeModuleQt::put_asset (const String &AssetId, ForgeObserver *observer) {

   UInt64 requestId (0);
   
   if (_state.networkAccessManager && observer) {
      
      requestId = _state.requestCounter++;

      AssetStruct *as = _state.assetTable.lookup (AssetId);
      if (as) {
      
         String assetJson;
      
         if (_asset_to_json (AssetId, assetJson)) {
         
            _state.obsTable.store (requestId, observer);
         
            QUrl url (_state.baseUrl);
            QString path ("/%1/%2");
            url.setPath (path.arg (_state.db).arg (AssetId.get_buffer ()));

            QByteArray byteArray (assetJson.get_buffer ());
         
            QNetworkRequest request (url);
            request.setRawHeader ("User-Agent", ForgeUserAgentName);
            request.setRawHeader ("Accept", "application/json");
            request.setAttribute (LocalAttrId, requestId);
            request.setAttribute (LocalAttrType, ForgePutAsset);

            QNetworkReply *reply = _state.networkAccessManager->put (request, byteArray);
         }
      }
      else { _handle_not_found (AssetId, requestId, ForgePutAsset, observer); }
   }
   
   return requestId;
}


dmz::UInt64
dmz::ForgeModuleQt::delete_asset (const String &AssetId, ForgeObserver *observer) {

   UInt64 requestId (0);

   if (_state.networkAccessManager && observer) {
      
      requestId = _state.requestCounter++;

      AssetStruct *as = _state.assetTable.remove (AssetId);
      if (as) {

         _state.obsTable.store (requestId, observer);
         
         QUrl url (_state.baseUrl);
         QString path ("/%1/%2");
         url.setPath (path.arg (_state.db).arg (AssetId.get_buffer ()));

         url.addQueryItem ("rev", as->revision.get_buffer ());

         QNetworkRequest request (url);
         request.setRawHeader("User-Agent", ForgeUserAgentName);
         request.setRawHeader ("Accept", "application/json");
         request.setAttribute (LocalAttrId, requestId);
         request.setAttribute (LocalAttrType, ForgeDeleteAsset);

         QNetworkReply *reply = _state.networkAccessManager->deleteResource (request);
         delete as; as = 0;
      }
      else { _handle_not_found (AssetId, requestId, ForgeDeleteAsset, observer); }
   }
   
   return requestId;
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

   UInt64 requestId (0);

   if (observer) {
      
      requestId = _state.requestCounter++;
      
      AssetStruct *as = _state.assetTable.lookup (AssetId);
      if (as) {

         //as->preview.append (Preview);
         
         _state.obsTable.store (requestId, observer);
         
         UploadStruct *us = new UploadStruct;
         us->requestId = requestId;
         us->type = ForgePutAssetMedia;
         us->assetId = AssetId;
         us->files.append (File);
         us->mimeType = MimeType;
         
         _state.uploadQueue.enqueue (us);
         
         QTimer::singleShot (0, this, SLOT (_start_next_upload ()));
      }
      else { _handle_not_found (AssetId, requestId, ForgePutAssetMedia, observer); }
   }
   
   return requestId;
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
dmz::ForgeModuleQt::add_asset_preview (
      const String &AssetId,
      const StringContainer &Files,
      ForgeObserver *observer) {

   UInt64 requestId (0);

   if (observer) {
      
      requestId = _state.requestCounter++;
      
      AssetStruct *as = _state.assetTable.lookup (AssetId);
      if (as) {

         //as->preview.append (Preview);
         
         _state.obsTable.store (requestId, observer);

         UploadStruct *us = new UploadStruct;
         us->requestId = requestId;
         us->type = ForgeAddAssetPreview;
         us->assetId = AssetId;
         us->files = Files;

         _state.uploadQueue.enqueue (us);

         QTimer::singleShot (0, this, SLOT (_start_next_upload ()));
      }
      else { _handle_not_found (AssetId, requestId, ForgeAddAssetPreview, observer); }
   }
   
   return requestId;
}


dmz::UInt64
dmz::ForgeModuleQt::remove_asset_preview (
      const String &AssetId,
      const StringContainer &Files,
      ForgeObserver *observer) {

   UInt64 requestId (0);

   // if (observer) {
   //    
   //    requestId = _state.requestCounter++;
   //    
   //    AssetStruct *as = _state.assetTable.lookup (AssetId);
   //    if (as) {
   // 
   //       as->preview.append (Preview);
   //       
   //       _state.obsTable.store (requestId, observer);
   // 
   //       UploadStruct *us = new UploadStruct;
   //       us->requestId = requestId;
   //       us->assetId = AssetId;
   //       us->file = Preview;
   // 
   //       _state.uploadQueue.enqueue (us);
   // 
   //       QTimer::singleShot (0, this, SLOT (_start_next_upload ()));
   //    }
   //    else { _handle_not_found (AssetId, requestId, ForgeAddAssetPreview, observer); }
   // }
   
   return requestId;
}


void
dmz::ForgeModuleQt::_reply_finished (QNetworkReply *reply) {
   
   if (reply) {

      const Int32 StatusCode =
         reply->attribute (QNetworkRequest::HttpStatusCodeAttribute).toInt();

      QNetworkRequest request = reply->request ();
      
      const Int32 Type = request.attribute (LocalAttrType).toInt ();
      const UInt64 Id = request.attribute (LocalAttrId).toULongLong ();

      QString data (reply->readAll ());
      const String JsonData (qPrintable (data));
      
// _state.log.warn << "_reply_finished[" << StatusCode << "]: " << Id << endl;

      if (!JsonData) {

         String msg ("Network Error: ");
         msg << qPrintable (reply->errorString ());
         
         _handle_error (Id, (ForgeTypeEnum)Type, msg);
      }
      else {
         
// _state.log.warn << "json: " << JsonData << endl;

         switch (Type) {

            case ForgeSearch:
_state.log.warn << "<-- ForgeSearch" << endl;
               _handle_search (Id, JsonData);
               break;
         
            case ForgeGetAsset:
_state.log.warn << "<-- ForgeGetAsset" << endl;
               _handle_get_asset (Id, JsonData);
               break;
            
            case ForgePutAsset:
                  _handle_put_asset (Id, JsonData);
_state.log.warn << "<-- ForgePutAsset" << endl;
               break;
         
            case ForgeDeleteAsset:
_state.log.warn << "<-- ForgeDeleteAsset" << endl;
               _handle_delete_asset (Id, JsonData);
               break;
         
            case ForgeGetAssetMedia:
            break;
         
            case ForgePutAssetMedia:
_state.log.warn << "<-- ForgePutAssetMedia" << endl;
               _handle_put_asset_media (Id, JsonData);
               break;
         
            case ForgeGetAssetPreview:
            break;
         
            case ForgeAddAssetPreview:
_state.log.warn << "<-- ForgeAddAssetPreview" << endl;
               _handle_add_asset_preview (Id, JsonData);
               break;
         
            case LocalGetUuids:
               _handle_get_uuids (Id, JsonData);
               break;
               
            // case LocalUpdateAsset:
            //    _handle_update_asset (Id, JsonData);
            //    break;
         
            default:
               String msg ("Unknown request type: ");
               msg << Type;
            
               _state.log.warn << msg << endl;   

               _handle_error (Id, (ForgeTypeEnum)Type, msg);
         }
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
      
      reply->deleteLater ();
   }
}


void
dmz::ForgeModuleQt::_upload_progress (qint64 bytesSent, qint64 bytesTotal) {

   // QNetworkReply *reply = (QNetworkReply *)sender ();
   if (_state.uploadReply) {
      
      QNetworkRequest request = _state.uploadReply->request ();
      const UInt64 Id = request.attribute (LocalAttrId).toULongLong ();
// _state.log.error << "_upload_progress["  << Id  << "]: " << bytesSent << " of " << bytesTotal << endl;
   }
}


void
dmz::ForgeModuleQt::_upload_finished () {

   if (_state.uploadReply) {
      
      QNetworkRequest request = _state.uploadReply->request ();
      const UInt64 Id = request.attribute (LocalAttrId).toULongLong ();
      
      _state.uploadReply->deleteLater ();
      _state.uploadReply = 0;
   }
   
   if (_state.uploadFile) {
      
      _state.uploadFile->close ();
      _state.uploadFile->deleteLater ();
      _state.uploadFile = 0;
   }

   if (_state.upload) {
      
      if (_state.upload->files.get_count () == 0) { _state.upload = 0; }
   }
   
   _start_next_upload ();
}


void
dmz::ForgeModuleQt::_start_next_upload () {

   // only upload next file if not currently uploading
   if (!_state.uploadFile && !_state.uploadReply) {

      // get next set of file to upload from queue
      if (!_state.upload && !_state.uploadQueue.isEmpty ()) {
         
         _state.upload = _state.uploadQueue.dequeue ();
      }
      
      if (_state.upload && _state.networkAccessManager) {

         String revision;
         if (_lookup_revision (_state.upload->assetId, revision)) {

            String file;
            _state.upload->files.get_first (file);
            _state.upload->files.remove (file);
                        
            QFileInfo fi (file.get_buffer ());

            QUrl url (_state.baseUrl);
            QString path ("/%1/%2/%3");
            
            url.setPath (
               path.arg (_state.db)
                   .arg (_state.upload->assetId.get_buffer ())
                   .arg (fi.fileName ()));

            url.addQueryItem ("rev", revision.get_buffer ());

            const QString Ext (fi.suffix ().toLower ());
            QByteArray mimeType (LocalDefaultMimeType);
            if (localMimeTypes.contains (Ext)) { mimeType = localMimeTypes[Ext]; }

            QNetworkRequest request (url);
            request.setRawHeader ("User-Agent", ForgeUserAgentName);
            request.setRawHeader ("Accept", "application/json");
            request.setHeader (QNetworkRequest::ContentTypeHeader, mimeType);
            request.setAttribute (LocalAttrId, _state.upload->requestId);
            request.setAttribute (LocalAttrType, _state.upload->type);

            _state.uploadFile = new QFile (fi.absoluteFilePath ());

            if (_state.uploadFile->open (QIODevice::ReadOnly)) {

               _state.uploadReply =
                  _state.networkAccessManager->put (request, _state.uploadFile);
            
               connect (
                  _state.uploadReply, SIGNAL (uploadProgress (qint64, qint64)),
                  this, SLOT (_upload_progress (qint64, qint64)));
               
               connect (
                  _state.uploadReply, SIGNAL (finished ()),
                  this, SLOT (_upload_finished ()));
            }
            else {

               _state.log.warn << "Fialed to open file: "
                               << qPrintable (fi.absoluteFilePath ()) << endl;
                               
               delete _state.uploadFile;
               _state.uploadFile = 0;
            }
         } 
      }
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

      String error = config_to_string ("error", global);
      if (error) {
         
         _handle_error (RequestId, ForgeGetAsset, config_to_string ("reason", global));
      }
      else {
         
         const String AssetId (config_to_string (LocalId, global));

         _config_to_asset (AssetId, global);

         StringContainer container;
         container.append (JsonData);

         _handle_reply (RequestId, ForgeGetAsset, container);
      }
   }
   else {
      
      _handle_error (RequestId, ForgeGetAsset, LocalJsonParseErrorMessage);
   }
}


void
dmz::ForgeModuleQt::_handle_put_asset (const UInt64 RequestId, const String &JsonData) {

   Config global ("global");
   
   if (json_string_to_config (JsonData, global)) { 

      if (config_to_boolean ("ok", global)) {
         
         const String AssetId (config_to_string ("id", global));
         const String Revision (config_to_string ("rev", global));
         _store_revision (AssetId, Revision);

         StringContainer container;
         container.append (JsonData);

         _handle_reply (RequestId, ForgePutAsset, container);
      }
      else {
         
         _handle_error (RequestId, ForgePutAsset, config_to_string ("reason", global));
      }
   }
   else {
      
      _handle_error (RequestId, ForgePutAsset, LocalJsonParseErrorMessage);
   }
}


void
dmz::ForgeModuleQt::_handle_delete_asset (const UInt64 RequestId, const String &JsonData) {

   Config global ("global");
   
   if (json_string_to_config (JsonData, global)) { 

      if (config_to_boolean ("ok", global)) {
         
         const String AssetId (config_to_string ("id", global));
         const String Revision (config_to_string ("rev", global));
         _store_revision (AssetId, Revision);

         StringContainer container;
         container.append (JsonData);

         _handle_reply (RequestId, ForgeDeleteAsset, container);
      }
      else {
         
         _handle_error (RequestId, ForgeDeleteAsset, config_to_string ("reason", global));
      }
   }
   else {
      
      _handle_error (RequestId, ForgeDeleteAsset, LocalJsonParseErrorMessage);
   }
}


void
dmz::ForgeModuleQt::_handle_put_asset_media (
      const UInt64 RequestId,
      const String &JsonData) {

   Boolean finished (False);
   if (_state.upload && (_state.upload->requestId == RequestId)) {
      
      if (_state.upload->files.get_count () == 0) { finished = True; }
   }

   Config global ("global");
   
   if (json_string_to_config (JsonData, global)) { 

      if (config_to_boolean ("ok", global)) {
         
         const String AssetId (config_to_string ("id", global));
         const String Revision (config_to_string ("rev", global));
         _store_revision (AssetId, Revision);

         StringContainer container;
         container.append (JsonData);

         if (finished) {
            
            _handle_reply (RequestId, _state.upload->type, container);
         }
      }
      else {
         
         if (_state.upload && (_state.upload->requestId == RequestId)) {

            _state.upload->files.clear ();
         }
         
         _handle_error (
            RequestId,
            _state.upload->type,
            config_to_string ("reason", global));
      }
   }
   else {
      
      _handle_error (RequestId, _state.upload->type, LocalJsonParseErrorMessage);
   }
}


void
dmz::ForgeModuleQt::_handle_add_asset_preview (
      const UInt64 RequestId,
      const String &JsonData) {

   Boolean finished (False);
   if (_state.upload && (_state.upload->requestId == RequestId)) {
      
      if (_state.upload->files.get_count () == 0) { finished = True; }
   }

   Config global ("global");
   
   if (json_string_to_config (JsonData, global)) { 

      if (config_to_boolean ("ok", global)) {
         
         const String AssetId (config_to_string ("id", global));
         const String Revision (config_to_string ("rev", global));
         _store_revision (AssetId, Revision);

         StringContainer container;
         container.append (JsonData);

         if (finished) {
            
            _handle_reply (RequestId, ForgeAddAssetPreview, container);
         }
      }
      else {
         
         if (_state.upload && (_state.upload->requestId == RequestId)) {

            _state.upload->files.clear ();
         }
         
         _handle_error (
            RequestId,
            ForgeAddAssetPreview,
            config_to_string ("reason", global));
      }
   }
   else {
      
      _handle_error (RequestId, ForgeAddAssetPreview, LocalJsonParseErrorMessage);
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
      }
   }
   else {
      
      _state.log.warn << "[" << RequestId << "] " << LocalJsonParseErrorMessage << endl;
   }
}


void
dmz::ForgeModuleQt::_handle_not_found (
      const String &AssetId,
      const UInt64 RequestId,
      const ForgeTypeEnum &RequestType,
      ForgeObserver *observer) {

   if (observer) {
      
      _state.obsTable.store (RequestId, observer);
      
      String msg ("Not found: ");
      msg << AssetId;
      
      _handle_error (RequestId, RequestType, msg);
   }
}


void
dmz::ForgeModuleQt::_handle_reply (
      const UInt64 RequestId,
      const ForgeTypeEnum &RequestType,
      StringContainer &Container) {

   ForgeObserver *observer = _state.obsTable.remove (RequestId);
   if (observer) {
      
      observer->handle_reply (RequestId, RequestType, False, Container);
   }
}


void
dmz::ForgeModuleQt::_handle_error (
      const UInt64 RequestId,
      const ForgeTypeEnum &RequestType,
      const String &Message) {

   ForgeObserver *observer = _state.obsTable.remove (RequestId);
   if (observer) {
      
      StringContainer container;
      container.append (Message);
      
      observer->handle_reply (RequestId, RequestType, True, container);
   }
}


dmz::Boolean
dmz::ForgeModuleQt::_store_revision (const String &AssetId, const String &Value) {
   
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
         url.addQueryItem ("count", QString::number (10));

         QNetworkRequest request (url);
         request.setRawHeader ("User-Agent", ForgeUserAgentName);
         request.setRawHeader ("Accept", "application/json");
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
   
   Config webservice;
   if (local.lookup_config ("webservice", webservice)) {
      
      Config proxy;
      if (webservice.lookup_config ("proxy", proxy)) {

         String host = config_to_string ("host", proxy, "localhost");
         Int32 port = config_to_int32 ("port", proxy, 8888);

         QNetworkProxy proxy;
         proxy.setType (QNetworkProxy::HttpProxy);
         proxy.setHostName (host.get_buffer ());
         proxy.setPort (port);

         QNetworkProxy::setApplicationProxy(proxy);

         _state.log.info << "Using proxy: " << host << ":" << port << endl;
      }
      
      String host = qPrintable (_state.baseUrl.host ());
      host =  config_to_string ("host", webservice, host);
      _state.baseUrl.setHost (host.get_buffer ());

      Int32 port = _state.baseUrl.port ();
      port = config_to_int32 ("port", webservice, port);
      _state.baseUrl.setPort (port);

      _state.log.info
         << "Using API endpoint: " << qPrintable (_state.baseUrl.toString ()) << endl;

      _state.db = config_to_string ("db", webservice, "assets").get_buffer ();
   }
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
