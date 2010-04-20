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

   static const dmz::String Local_Id ("_id");
   static const dmz::String Local_Rev ("_rev");
   static const dmz::String Local_Attachments ("_attachments");
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
   static const dmz::String LocalOriginalName ("original_name");
   static const dmz::String LocalValue ("value");

   static const char LocalId[] = "id";
   static const char LocalRev[] = "rev";
   static char LocalAccept[] = "Accept";
   static char LocalApplicationJson[] = "application/json";
   static char LocalApplicationOctetStream[] = "application/octet-stream";
   static char LocalUserAgent[] = "User-Agent";
   static char LocalMimeIVE[] = "model/x-ive";

   static const dmz::Int32 LocalGetUuids              = dmz::ForgeTypeUser + 1;
   static const dmz::Int32 LocalPutAssetMediaPhase1   = dmz::ForgeTypeUser + 2;
   static const dmz::Int32 LocalPutAssetMediaPhase2   = dmz::ForgeTypeUser + 3;
   static const dmz::Int32 LocalAddAssetPreviewPhase1 = dmz::ForgeTypeUser + 4;
   static const dmz::Int32 LocalAddAssetPreviewPhase2 = dmz::ForgeTypeUser + 5;
   static const dmz::Int32 LocalAddAssetPreviewPhase3 = dmz::ForgeTypeUser + 6;
   
   static QNetworkRequest::Attribute LocalAttrType =
      (QNetworkRequest::Attribute) (QNetworkRequest::User + 1);
      
   static QNetworkRequest::Attribute LocalAttrId =
      (QNetworkRequest::Attribute) (QNetworkRequest::User + 2);
   
   static QHash<QString, QByteArray> localMimeTypes;
   
   struct UploadStruct {
      
      dmz::UInt64 requestId;
      dmz::Int32 requestType;
      dmz::String assetId;
      dmz::StringContainer files;
      dmz::String mimeType;
      
      UploadStruct () : requestId (0), requestType (0) {;}
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
   QMap<QString, String> current;
   QMap<QString, String> originalName;
   Config attachments;
   
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
   localMimeTypes["collada"]  = "model/x-collada+xml";
   localMimeTypes["ive"]      = LocalMimeIVE;
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
      request.setRawHeader (LocalUserAgent, ForgeUserAgentName);
      request.setRawHeader (LocalAccept, LocalApplicationJson);
      request.setAttribute (LocalAttrId, requestId);
      request.setAttribute (LocalAttrType, (int)ForgeTypeSearch);

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
      
      QNetworkReply *reply = _get_asset (AssetId, requestId, ForgeTypeGetAsset);
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

         _state.obsTable.store (requestId, observer);
         
         QNetworkReply *reply = _put_asset (AssetId, requestId, ForgeTypePutAsset);
      }
      else { _handle_not_found (AssetId, requestId, ForgeTypePutAsset, observer); }
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

         url.addQueryItem (LocalRev, as->revision.get_buffer ());

         QNetworkRequest request (url);
         request.setRawHeader (LocalUserAgent, ForgeUserAgentName);
         request.setRawHeader (LocalAccept, LocalApplicationJson);
         request.setAttribute (LocalAttrId, requestId);
         request.setAttribute (LocalAttrType, (int)ForgeTypeDeleteAsset);

         QNetworkReply *reply = _state.networkAccessManager->deleteResource (request);
         delete as; as = 0;
      }
      else { _handle_not_found (AssetId, requestId, ForgeTypePutAsset, observer); }
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

         //as->current.store (File);
         
         _state.obsTable.store (requestId, observer);
         
         UploadStruct *us = new UploadStruct;
         us->requestId = requestId;
         us->requestType = LocalPutAssetMediaPhase1;
         us->assetId = AssetId;
         us->files.append (File);
         us->mimeType = MimeType;
         
         _state.uploadQueue.enqueue (us);
         
         QTimer::singleShot (0, this, SLOT (_start_next_upload ()));
      }
      else { _handle_not_found (AssetId, requestId, ForgeTypePutAssetMedia, observer); }
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

         _state.obsTable.store (requestId, observer);

         UploadStruct *us = new UploadStruct;
         us->requestId = requestId;
         us->requestType = LocalAddAssetPreviewPhase1;
         us->assetId = AssetId;
         us->files = Files;

         _state.uploadQueue.enqueue (us);

         QTimer::singleShot (0, this, SLOT (_start_next_upload ()));
      }
      else { _handle_not_found (AssetId, requestId, ForgeTypeAddAssetPreview, observer); }
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
   //    else { _handle_not_found (AssetId, requestId, ForgeTypeAddAssetPreview, observer); }
   // }
   
   return requestId;
}


void
dmz::ForgeModuleQt::_reply_finished (QNetworkReply *reply) {
   
   if (reply) {

      const Int32 StatusCode =
         reply->attribute (QNetworkRequest::HttpStatusCodeAttribute).toInt();

      QNetworkRequest request = reply->request ();
      
      const Int32 RequestType = request.attribute (LocalAttrType).toInt ();
      const UInt64 RequestId = request.attribute (LocalAttrId).toULongLong ();

      QString data (reply->readAll ());
      const String JsonData (qPrintable (data));
      
// _state.log.warn << "_reply_finished[" << StatusCode << "]: " << Id << endl;

      if (!JsonData) {

         String msg ("Network Error: ");
         msg << qPrintable (reply->errorString ());
         
         _handle_error (RequestId, RequestType, msg);
      }
      else {
         
// _state.log.warn << "json: " << JsonData << endl;

         switch (RequestType) {

            case ForgeTypeSearch:
_state.log.warn << "<-- ForgeTypeSearch" << endl;
               _handle_search (RequestId, JsonData);
               break;
         
            case ForgeTypeGetAsset:
_state.log.warn << "<-- ForgeTypeGetAsset" << endl;
               _handle_get_asset (RequestId, JsonData);
               break;
            
            case ForgeTypePutAsset:
                  _handle_put_asset (RequestId, JsonData);
_state.log.warn << "<-- ForgeTypePutAsset" << endl;
               break;
         
            case ForgeTypeDeleteAsset:
_state.log.warn << "<-- ForgeTypePutAsset" << endl;
               _handle_delete_asset (RequestId, JsonData);
               break;
         
            case ForgeTypeGetAssetMedia:
            break;
         
            case LocalPutAssetMediaPhase1:
_state.log.warn << "<-- LocalPutAssetMediaPhase1" << endl;
               _handle_put_asset_media_phase1 (RequestId, JsonData);
               break;

            case LocalPutAssetMediaPhase2:
_state.log.warn << "<-- LocalPutAssetMediaPhase2" << endl;
               _handle_put_asset_media_phase2 (RequestId, JsonData);
               break;
         
            case ForgeTypeGetAssetPreview:
            break;
            
            case LocalAddAssetPreviewPhase1:
_state.log.warn << "<-- LocalAddAssetPreviewPhase1" << endl;
               _handle_add_asset_preview_phase1 (RequestId, JsonData);
               break;
         
            case LocalAddAssetPreviewPhase2:
_state.log.warn << "<-- LocalAddAssetPreviewPhase2" << endl;
               _handle_add_asset_preview_phase2 (RequestId, JsonData);
               break;

            case LocalAddAssetPreviewPhase3:
_state.log.warn << "<-- LocalAddAssetPreviewPhase3" << endl;
               _handle_add_asset_preview_phase3 (RequestId, JsonData);
               break;

            case LocalGetUuids:
               _handle_get_uuids (RequestId, JsonData);
               break;
               
            // case LocalUpdateAsset:
            //    _handle_update_asset (RequestId, JsonData);
            //    break;
         
            default:
               String msg ("Unknown request type: ");
               msg << RequestType;
            
               _state.log.warn << msg << endl;   

               _handle_error (RequestId, RequestType, msg);
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
            
            const QString Ext (fi.suffix ().toLower ());

            QString attachmentName = fi.fileName ();
            
            if (_state.upload->requestType == LocalPutAssetMediaPhase1) {
            
               attachmentName = _add_asset_media_file (_state.upload->assetId, file);
            }
            else if (_state.upload->requestType == LocalAddAssetPreviewPhase1) {
               
               attachmentName = _add_asset_preview_file (_state.upload->assetId, file);
            }

qDebug () << "attachmentName: " << attachmentName;
            
            QByteArray mimeType (LocalApplicationOctetStream);
            if (_state.upload->mimeType) { mimeType = _state.upload->mimeType.get_buffer (); }
            else if (localMimeTypes.contains (Ext)) { mimeType = localMimeTypes[Ext]; }

            QUrl url (_state.baseUrl);
            QString path ("/%1/%2/%3");
            
            url.setPath (
               path.arg (_state.db)
                   .arg (_state.upload->assetId.get_buffer ())
                   .arg (attachmentName));

            url.addQueryItem (LocalRev, revision.get_buffer ());

            QNetworkRequest request (url);
            request.setRawHeader (LocalUserAgent, ForgeUserAgentName);
            request.setRawHeader (LocalAccept, LocalApplicationJson);
            request.setHeader (QNetworkRequest::ContentTypeHeader, mimeType);
            request.setAttribute (LocalAttrId, _state.upload->requestId);
            request.setAttribute (LocalAttrType, (int)_state.upload->requestType);

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

            String assetId = config_to_string (LocalId, cd);
            if (assetId) { container.append (assetId); }
         }
      }
      
      _handle_reply (RequestId, ForgeTypeSearch, container);
   }
   else {
      
      _handle_error (RequestId, ForgeTypeSearch, LocalJsonParseErrorMessage);
   }
}


void
dmz::ForgeModuleQt::_handle_get_asset (const UInt64 RequestId, const String &JsonData) {

   Config global ("global");
   
   if (json_string_to_config (JsonData, global)) { 

      String error = config_to_string ("error", global);
      if (error) {
         
         _handle_error (RequestId, ForgeTypeGetAsset, config_to_string ("reason", global));
      }
      else {
         
         _config_to_asset (global);

         StringContainer container;
         container.append (JsonData);

         _handle_reply (RequestId, ForgeTypeGetAsset, container);
      }
   }
   else {
      
      _handle_error (RequestId, ForgeTypeGetAsset, LocalJsonParseErrorMessage);
   }
}


void
dmz::ForgeModuleQt::_handle_put_asset (const UInt64 RequestId, const String &JsonData) {

   Config global ("global");
   
   if (json_string_to_config (JsonData, global)) { 

      if (config_to_boolean ("ok", global)) {
         
         const String AssetId (config_to_string (LocalId, global));
         const String Revision (config_to_string (LocalRev, global));
         _store_revision (AssetId, Revision);

         StringContainer container;
         container.append (JsonData);

         _handle_reply (RequestId, ForgeTypePutAsset, container);
      }
      else {
         
         _handle_error (RequestId, ForgeTypePutAsset, config_to_string ("reason", global));
      }
   }
   else {
      
      _handle_error (RequestId, ForgeTypePutAsset, LocalJsonParseErrorMessage);
   }
}


void
dmz::ForgeModuleQt::_handle_delete_asset (const UInt64 RequestId, const String &JsonData) {

   Config global ("global");
   
   if (json_string_to_config (JsonData, global)) { 

      if (config_to_boolean ("ok", global)) {
         
         const String AssetId (config_to_string (LocalId, global));
         const String Revision (config_to_string (LocalRev, global));
         _store_revision (AssetId, Revision);

         StringContainer container;
         container.append (JsonData);

         _handle_reply (RequestId, ForgeTypePutAsset, container);
      }
      else {
         
         _handle_error (RequestId, ForgeTypePutAsset, config_to_string ("reason", global));
      }
   }
   else {
      
      _handle_error (RequestId, ForgeTypePutAsset, LocalJsonParseErrorMessage);
   }
}


void
dmz::ForgeModuleQt::_handle_put_asset_media_phase1 (
      const UInt64 RequestId,
      const String &JsonData) {

   Boolean finished (False);
   if (_state.upload && (_state.upload->requestId == RequestId)) {
      
      if (_state.upload->files.get_count () == 0) { finished = True; }
   }

   Config global ("global");
   
   if (json_string_to_config (JsonData, global)) { 

      if (config_to_boolean ("ok", global)) {
         
         const String AssetId (config_to_string (LocalId, global));
         const String Revision (config_to_string (LocalRev, global));
         _store_revision (AssetId, Revision);

         if (finished) { _get_asset (AssetId, RequestId, LocalPutAssetMediaPhase2); }
      }
      else {
         
         if (_state.upload && (_state.upload->requestId == RequestId)) {

            _state.upload->files.clear ();
         }
         
         _handle_error (
            RequestId,
            ForgeTypePutAssetMedia,
            config_to_string ("reason", global));
      }
   }
   else {
      
      _handle_error (RequestId, ForgeTypePutAssetMedia, LocalJsonParseErrorMessage);
   }
}

void
dmz::ForgeModuleQt::_handle_put_asset_media_phase2 (
      const UInt64 RequestId,
      const String &JsonData) {

   Config global ("global");

   if (json_string_to_config (JsonData, global)) { 

      String error = config_to_string ("error", global);
      if (error) {

         _handle_error (RequestId, ForgeTypePutAssetMedia, config_to_string ("reason", global));
      }
      else {

         _config_to_asset (global);

         StringContainer container;
         container.append (JsonData);

         _handle_reply (RequestId, ForgeTypePutAsset, container);
      }
   }
   else {

      _handle_error (RequestId, ForgeTypePutAssetMedia, LocalJsonParseErrorMessage);
   }
}


void
dmz::ForgeModuleQt::_handle_add_asset_preview_phase1 (
      const UInt64 RequestId,
      const String &JsonData) {

   Boolean finished (False);
   if (_state.upload && (_state.upload->requestId == RequestId)) {
      
      if (_state.upload->files.get_count () == 0) { finished = True; }
   }

   Config global ("global");
   
   if (json_string_to_config (JsonData, global)) { 

      if (config_to_boolean ("ok", global)) {

         const String AssetId (config_to_string (LocalId, global));
         const String Revision (config_to_string (LocalRev, global));
         _store_revision (AssetId, Revision);

         if (finished) { _get_asset (AssetId, RequestId, LocalAddAssetPreviewPhase2); }
      }
      else {

         if (_state.upload && (_state.upload->requestId == RequestId)) {

            _state.upload->files.clear ();
         }

         _handle_error (
            RequestId,
            ForgeTypeAddAssetPreview,
            config_to_string ("reason", global));
      }
   }
   else {
      
      _handle_error (RequestId, ForgeTypeAddAssetPreview, LocalJsonParseErrorMessage);
   }
}


void
dmz::ForgeModuleQt::_handle_add_asset_preview_phase2 (
      const UInt64 RequestId,
      const String &JsonData) {

   Config global ("global");
   
   if (json_string_to_config (JsonData, global)) { 

      const String AssetId (config_to_string (Local_Id, global));

      AssetStruct *as = _state.assetTable.lookup (AssetId);
      if (as){

         // _config_to_asset will overwrite previews so lets save it here so we don't loose it
         StringContainer previews = as->previews;
         _config_to_asset (global);

         // now lets restore previews and push to server
         as->previews = previews;
         _put_asset (AssetId, RequestId, LocalAddAssetPreviewPhase3);
      }
   }
   else {
      
      _handle_error (RequestId, ForgeTypeAddAssetPreview, LocalJsonParseErrorMessage);
   }
}


void
dmz::ForgeModuleQt::_handle_add_asset_preview_phase3 (
      const UInt64 RequestId,
      const String &JsonData) {

   Config global ("global");
   
   if (json_string_to_config (JsonData, global)) { 

      if (config_to_boolean ("ok", global)) {

         const String AssetId (config_to_string (LocalId, global));
         const String Revision (config_to_string (LocalRev, global));
         _store_revision (AssetId, Revision);

         StringContainer container;
         container.append (JsonData);

         _handle_reply (RequestId, ForgeTypeAddAssetPreview, container);
      }
      else {

         _handle_error (
            RequestId,
            ForgeTypeAddAssetPreview,
            config_to_string ("reason", global));
      }
   }
   else {
      
      _handle_error (RequestId, ForgeTypeAddAssetPreview, LocalJsonParseErrorMessage);
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
      const Int32 RequestType,
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
      const Int32 RequestType,
      StringContainer &Container) {

   ForgeObserver *observer = _state.obsTable.remove (RequestId);
   if (observer) {
      
      observer->handle_reply (RequestId, RequestType, False, Container);
   }
}


void
dmz::ForgeModuleQt::_handle_error (
      const UInt64 RequestId,
      const Int32 RequestType,
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


QNetworkReply *
dmz::ForgeModuleQt::_get_asset (
      const String &AssetId,
      const UInt64 RequestId,
      const Int32 RequestType) {

   QNetworkReply *reply (0);
   
   if (_state.networkAccessManager) {
      
      QUrl url (_state.baseUrl);
      QString path ("/%1/%2");
      url.setPath (path.arg (_state.db).arg (AssetId.get_buffer ()));

      QNetworkRequest request (url);
      request.setRawHeader (LocalUserAgent, ForgeUserAgentName);
      request.setRawHeader (LocalAccept, LocalApplicationJson);
      request.setAttribute (LocalAttrId, RequestId);
      request.setAttribute (LocalAttrType, (int)RequestType);

      reply = _state.networkAccessManager->get (request);
   }
   
   return reply;
}


QNetworkReply *
dmz::ForgeModuleQt::_put_asset (
      const String &AssetId,
      const UInt64 RequestId,
      const Int32 RequestType) {

   QNetworkReply *reply (0);
   
   String assetJson;
      
   if (_asset_to_json (AssetId, assetJson) && _state.networkAccessManager) {
      
      QUrl url (_state.baseUrl);
      QString path ("/%1/%2");
      url.setPath (path.arg (_state.db).arg (AssetId.get_buffer ()));

      QByteArray byteArray (assetJson.get_buffer ());
      
      QNetworkRequest request (url);
      request.setRawHeader (LocalUserAgent, ForgeUserAgentName);
      request.setRawHeader (LocalAccept, LocalApplicationJson);
      request.setAttribute (LocalAttrId, RequestId);
      request.setAttribute (LocalAttrType, (int)RequestType);

// qDebug () << "PUT: " << byteArray << endl;
      reply = _state.networkAccessManager->put (request, byteArray);
   }
   
   return reply;
}


QString
dmz::ForgeModuleQt::_add_asset_media_file (const String &AssetId, const String &File) {

   QFileInfo fi (File.get_buffer ());

   const String FileSHA = sha_from_file (File);
   QString current = FileSHA.get_buffer ();
   
   AssetStruct *as = _state.assetTable.lookup (AssetId);
   if (as) {
      
      const QString Ext (fi.suffix ().toLower ());

      QByteArray mimeType (LocalMimeIVE);
      if (localMimeTypes.contains (Ext)) { mimeType = localMimeTypes[Ext]; }
      
      Int32 rev = 1;
      
      current = QString ("%1-%2.%3").arg (rev).arg (current).arg (Ext);
      
      as->current[mimeType] = qPrintable (current);
      as->originalName[current] = qPrintable (fi.fileName ());

      as->type = ForgeAssetUnknown;
      
      if (Ext == QLatin1String ("ive")) {
         
         as->type = ForgeAsset3d;
      }
      else if (Ext == QLatin1String ("wav")) {
         
         as->type = ForgeAssetAudio;
      }
      else if (Ext == QLatin1String ("jpg") || Ext == QLatin1String ("png")) {
         
         as->type = ForgeAssetImage;
      }
      else if (Ext == QLatin1String ("xml") || Ext == QLatin1String ("json")) {
         
         as->type = ForgeAssetConfig;
      }
   }
   
   return current;
}


QString
dmz::ForgeModuleQt::_add_asset_preview_file (const String &AssetId, const String &File) {

   String path, name, ext;
   split_path_file_ext (File, path, name, ext);

   const String Preview (name + ext);
   
   AssetStruct *as = _state.assetTable.lookup (AssetId);
   if (as) { as->previews.append (Preview); }
   
   return Preview.get_buffer ();
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
      
      assetConfig.store_attribute (Local_Id, as->Id);

      if (as->revision) { assetConfig.store_attribute (Local_Rev, as->revision); }

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
      
      Config previewsConfig (LocalPreviews);
      String image;
      
      StringContainerIterator it;
      
      while (as->previews.get_next (it, image)) {
      
         Config data (LocalImages);
         data.store_attribute (LocalValue, image);
         
         previewsConfig.add_config (data);
      }
      
      assetConfig.overwrite_config (previewsConfig);
      
      assetConfig.overwrite_config (as->attachments);
      
      retVal = True;
   }
   
   return retVal;
}


dmz::Boolean
dmz::ForgeModuleQt::_config_to_asset (const Config &AssetConfig) {

   Boolean retVal (False);
   
   const String AssetId (config_to_string (Local_Id, AssetConfig));
   
   AssetStruct *as = _state.assetTable.lookup (AssetId);
   if (!as && AssetId) {
      
      create_asset (AssetId);
      as = _state.assetTable.lookup (AssetId);
   }
   
   if (as) {
      
      _store_revision (AssetId, config_to_string (Local_Rev, AssetConfig));
      
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

      as->previews.clear ();
      Config previewsConfig;
      
      if (AssetConfig.lookup_config (LocalPreviews, previewsConfig)) {
         
         Config imageList;

         if (previewsConfig.lookup_all_config (LocalImages, imageList)) {

            ConfigIterator it;
            Config image;

            while (imageList.get_next_config (it, image)) {

               as->previews.append (config_to_string (LocalValue, image));
            }
         }
      }
      
      AssetConfig.lookup_config (Local_Attachments, as->attachments);
      
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
         request.setRawHeader (LocalUserAgent, ForgeUserAgentName);
         request.setRawHeader (LocalAccept, LocalApplicationJson);
         request.setAttribute (LocalAttrId, requestId);
         request.setAttribute (LocalAttrType, (int)LocalGetUuids);

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
