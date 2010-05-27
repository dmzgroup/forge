#include <dmzApplicationState.h>
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
#include <QtSql/QtSql>


using namespace dmz;

namespace {
   
   QString
   local_get_current_rev (const String &Value) {

      Int32 rev (0);

      if (Value) {

         QString data (Value.get_buffer ());
         rev = data.section ('-', 0, 0).toInt ();
      }

      rev++;

      return QString::number (rev);
   }


   String
   local_get_hash (const String &Value) {

      QString hash;

      if (Value) {

         QFileInfo fi (Value.get_buffer ());
         QString data (fi.baseName ());
         hash = data.section ('-', 1, 1);
      }

      return qPrintable (hash);
   }

   static const char LocalApiEndpoint[] = "http://api.dmzforge.org:80";
   static const char LocalUserAgentName[] = "dmzForgeModuleQt";
   
   static const String LocalJsonParseErrorMessage ("Error parsing json response.");

   static const String Local_Id ("_id");
   static const String Local_Rev ("_rev");
   static const String Local_Attachments ("_attachments");
   static const String LocalName ("name");
   static const String LocalBrief ("brief");
   static const String LocalDetails ("details");
   static const String LocalKeywords ("keywords");
   static const String LocalThumbnails ("thumbnails");
   static const String LocalPreviews ("previews");
   static const String LocalImages ("images");
   static const String LocalType ("type");
   static const String LocalMedia ("media");
   static const String LocalMedia3d ("3d");
   static const String LocalMediaAudio ("audio");
   static const String LocalMediaImage ("image");
   static const String LocalMediaConfig ("config");
   static const String LocalCurrent ("current");
   static const String LocalOriginalName ("original_name");
   static const String LocalValue ("value");
   
   static const QString LocalGet ("get");
   static const QString LocalPut ("put");
   static const QString LocalDelete ("delete");

   static const char LocalId[] = "id";
   static const char LocalRev[] = "rev";
   static char LocalAccept[] = "Accept";
   static char LocalApplicationJson[] = "application/json";
   static char LocalApplicationOctetStream[] = "application/octet-stream";
   static char LocalUserAgent[] = "User-Agent";
   static char LocalMimeIVE[] = "model/x-ive";
   static char LocalETag[]= "ETag";
   static char LocalIfNoneMatch[] = "If-None-Match";

   static const Int32 LocalGetUuids              = ForgeTypeUser + 1;
   static const Int32 LocalPutAssetMediaPhase1   = ForgeTypeUser + 2;
   static const Int32 LocalPutAssetMediaPhase2   = ForgeTypeUser + 3;
   static const Int32 LocalPutAssetMediaPhase3   = ForgeTypeUser + 4;
   static const Int32 LocalAddAssetPreviewPhase1 = ForgeTypeUser + 5;
   static const Int32 LocalAddAssetPreviewPhase2 = ForgeTypeUser + 6;
   static const Int32 LocalAddAssetPreviewPhase3 = ForgeTypeUser + 7;
   
   static QNetworkRequest::Attribute LocalAttrType =
      (QNetworkRequest::Attribute) (QNetworkRequest::User + 1);
      
   static QNetworkRequest::Attribute LocalAttrId =
      (QNetworkRequest::Attribute) (QNetworkRequest::User + 2);
   
   static QHash<QString, QByteArray> localMimeTypes;
   
   struct UploadStruct {
      
      UInt64 requestId;
      Int32 requestType;
      String assetId;
      StringContainer files;
      String mimeType;
      
      UploadStruct () : requestId (0), requestType (0) {;}
   };
   
   struct DownloadStruct {
   
      UInt64 requestId;
      Int32 requestType;
      String assetId;
      String file;
      String hash;
      String targetFile;
      
      DownloadStruct () : requestId (0), requestType (0) {;}
   };
};

struct dmz::ForgeModuleQt::AssetStruct {

   const String Id;
   String revision;
   String type;
   String name;
   String brief;
   String details;
   StringContainer keywords;
   StringContainer media;
   StringContainer previews;
   QMap<QString, String> current;
   QMap<QString, String> originalName;
   Config attachments;
   
   AssetStruct (const String &AssetId) : Id (AssetId), type ("unknown") {;}
};


struct dmz::ForgeModuleQt::State {

   Log log;
   ApplicationState appState;
   QNetworkAccessManager *networkAccessManager;
   StringContainer uuids;
   UInt64 requestCounter;
   HashTableUInt64Template<ForgeObserver> obsTable;
   HashTableStringTemplate<AssetStruct> assetTable;
   QQueue<UploadStruct *> uploadQueue;
   QQueue<DownloadStruct *> downloadQueue;
   Boolean uploading;
   Boolean downloading;
   QUrl baseUrl;
   QString db;
   UploadStruct *upload;
   QNetworkReply *uploadReply;
   QFile *uploadFile;
   DownloadStruct *download;
   QNetworkReply *downloadReply;
   QTemporaryFile *downloadFile;
   String cacheDir;
   QSqlDatabase cacheDb;
   
   State (const PluginInfo &Info);
   ~State ();
   
   Boolean init_cache_dir ();
   Boolean init_cache_database ();
   Boolean exec (QSqlQuery &query);
   String get_etag (const String &AssetId, const String &Name);
   Boolean store_etag (const String &AssetId, const String &Name, const Boolean Value = False);
   Boolean update_etag (const String &AssetId, const String &Name);
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
      appState (Info),
      networkAccessManager (0),
      requestCounter (1000),
      uploading (False),
      baseUrl (LocalApiEndpoint),
      upload (0),
      uploadReply (0),
      uploadFile (0),
      download (0),
      downloadReply (0),
      downloadFile (0) {;}


dmz::ForgeModuleQt::State::~State () {

   if (cacheDb.isValid () && cacheDb.isOpen ()) { cacheDb.close (); }

   if (networkAccessManager) { networkAccessManager->deleteLater (); networkAccessManager = 0; }
   
   while (!uploadQueue.isEmpty ()) { delete uploadQueue.takeFirst (); }
   while (!downloadQueue.isEmpty ()) { delete downloadQueue.takeFirst (); }
   
   obsTable.clear ();
   assetTable.empty ();
}


dmz::Boolean
dmz::ForgeModuleQt::State::init_cache_dir () {

   Boolean retVal (False);
   cacheDir = get_home_directory ();
   
   if (is_valid_path (cacheDir)) {

#if defined (_WIN32)
      cacheDir << "/Local Settings/Application Data/";
#elif defined (__APPLE__) || defined (MACOSX)
      cacheDir << "/Library/Caches/";
#else
      cacheDir << "/.";
#endif

      cacheDir = format_path (cacheDir + "dmz/" + appState.get_app_name () + "/forge/");
      
      // create cache directory if it doesn't exists already
      if (!is_valid_path (cacheDir)) {
         
         create_directory (cacheDir);
         log.info << "Created applicatoin cache dir: " << cacheDir << endl;
      }
      
      retVal = True;
   }
   
   return retVal;
}


dmz::Boolean
dmz::ForgeModuleQt::State::init_cache_database () {

   Boolean retVal (False);
   
   if (is_valid_path (cacheDir)) {
   
      String dbName = format_path (cacheDir + "/cache.db");
      
      cacheDb = QSqlDatabase::addDatabase ("QSQLITE");
      cacheDb.setDatabaseName (dbName.get_buffer ());
   
      retVal = cacheDb.open ();
      
      if (retVal) {
         
         QSqlQuery query (cacheDb);
         QStringList tables = cacheDb.tables ();
         
         cacheDb.transaction ();
         
         if (!tables.contains("meta")) {
         
            query.prepare ("CREATE TABLE meta (schema_version INTEGER)");
            exec (query);

            query.prepare ("INSERT INTO meta (schema_version) VALUES (1)");
            exec (query);
         }
         
         if (!tables.contains ("previews")) {
            
            query.prepare (
               "CREATE TABLE previews ("
                  "id INTEGER PRIMARY KEY, "
                  "asset TEXT NOT NULL, "
                  "name TEXT NOT NULL, "
                  "etag TEXT NOT NULL"
               ");"
            );

            exec (query);
         }
         
         if (!cacheDb.commit ()) {
            
            log.warn << "SQL Error: " << qPrintable (cacheDb.lastError ().text ()) << endl;
         }
      }
      else {
         
         log.warn << "Database failed to open: " << dbName << endl;
      }
   }
   
   return retVal;
}


dmz::Boolean
dmz::ForgeModuleQt::State::exec (QSqlQuery &query) {

   Boolean retVal (query.exec ());
   
   if (query.lastError ().isValid ()) {
   
      log.debug << "SQL: " << qPrintable  (query.lastQuery ()) << endl;
      log.warn << "SQL Error: " << qPrintable (query.lastError ().text ()) << endl;
   }
   
   return retVal;
}


dmz::String
dmz::ForgeModuleQt::State::get_etag (const String &AssetId, const String &Name) {

   String etag;

   QSqlQuery query (cacheDb);
   
   query.prepare ("SELECT etag FROM previews WHERE asset = :asset AND name = :name");
   query.bindValue (":asset", AssetId.get_buffer ());
   query.bindValue (":name", Name.get_buffer ());
   exec (query);
   
   if (query.next ()) { etag = qPrintable (query.value (0).toString ()); }

   return etag;
}


dmz::Boolean
dmz::ForgeModuleQt::State::store_etag (
      const String &AssetId,
      const String &Name,
      const Boolean Update) {

   String revision;
   AssetStruct *asset = assetTable.lookup (AssetId);
   if (asset) { revision = asset->revision; }

   QSqlQuery query (cacheDb);

   if (Update) {
      
      query.prepare ("UPDATE previews SET etag = :etag WHERE asset = :asset AND name = :name");
   }
   else {
      
      query.prepare ("INSERT INTO previews (asset, name, etag) VALUES (:asset, :name, :etag)");
   }
   
   query.bindValue (":asset", AssetId.get_buffer ());
   query.bindValue (":name", Name.get_buffer ());
   query.bindValue (":etag", revision.get_buffer ());
   
   return exec (query);
}


dmz::Boolean
dmz::ForgeModuleQt::State::update_etag (const String &AssetId, const String &Name) {

   return store_etag (AssetId, Name, True);
}


dmz::ForgeModuleQt::ForgeModuleQt (const PluginInfo &Info, Config &local) :
      QObject (0),
      Plugin (Info),
      ForgeModule (Info),
      _state (*(new State (Info))) {

   local_init_mime_types ();

   _state.networkAccessManager = new QNetworkAccessManager (this);

   _init (local);
   
   _state.init_cache_dir ();
   _state.init_cache_database ();
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
   
   AssetStruct *asset = _state.assetTable.lookup (AssetId);
   
   if (!asset) {
   
      asset = new AssetStruct (AssetId ? AssetId : _get_uuid ());
      
      if (!_state.assetTable.store (asset->Id, asset)) {
         
         delete asset;
         asset = 0;
      }
   }
   
   if (asset) { retVal = asset->Id; }
   
   return retVal;
}


dmz::ForgeAssetTypeEnum
dmz::ForgeModuleQt::lookup_asset_type (const String &AssetId) {

   ForgeAssetTypeEnum retVal (ForgeAssetUnknown);

   AssetStruct *asset = _state.assetTable.lookup (AssetId);
   if (asset) {
   
      if (asset->type == LocalMedia3d) { retVal = ForgeAsset3d; }
      else if (asset->type == LocalMediaAudio) { retVal = ForgeAssetAudio; }
      else if (asset->type == LocalMediaConfig) { retVal = ForgeAssetConfig; }
      else if (asset->type == LocalMediaImage) { retVal = ForgeAssetImage; }
   }
   
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

      QNetworkReply *reply = _request (LocalGet, url, requestId, ForgeTypeSearch);
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

      AssetStruct *asset = _state.assetTable.lookup (AssetId);
      if (asset) {

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

      AssetStruct *asset = _state.assetTable.remove (AssetId);
      if (asset) {

         _state.obsTable.store (requestId, observer);
         
         QUrl url (_state.baseUrl);
         QString path ("/%1/%2");
         url.setPath (path.arg (_state.db).arg (AssetId.get_buffer ()));

         url.addQueryItem (LocalRev, asset->revision.get_buffer ());

         QNetworkReply *reply = _request (LocalDelete, url, requestId, ForgeTypeDeleteAsset);

         delete asset; asset = 0;
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

   UInt64 requestId (0);

   if (observer) {
      
      requestId = _state.requestCounter++;
      
      AssetStruct  *asset = _state.assetTable.lookup (AssetId);
      if (asset) {
         
         _state.obsTable.store (requestId, observer);
         
         String targetFile = AssetId + "-" + File;
         targetFile = format_path (_state.cacheDir + targetFile);

         if (is_valid_path (targetFile)) {
         
            StringContainer container;
            container.append (targetFile);

            _handle_reply (requestId, ForgeTypeGetAssetMedia, container);
         }
         else {
            
            DownloadStruct *ds = new DownloadStruct;
            ds->requestId = requestId;
            ds->requestType = ForgeTypeGetAssetMedia;
            ds->assetId = AssetId;
            ds->file = File;
            ds->hash = local_get_hash (File);
            ds->targetFile = targetFile;

            _state.downloadQueue.enqueue (ds);

            QTimer::singleShot (0, this, SLOT (_start_next_download ()));
         }
      }
      else { _handle_not_found (AssetId, requestId, ForgeTypeGetAssetMedia, observer); }
   }

   return requestId;
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
      
      AssetStruct *asset = _state.assetTable.lookup (AssetId);
      if (asset) {

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

   UInt64 requestId (0);

   
   if (observer) {
      
      requestId = _state.requestCounter++;
      
      AssetStruct  *asset = _state.assetTable.lookup (AssetId);
      if (asset) {
         
         _state.obsTable.store (requestId, observer);
         
         const String ETag (_state.get_etag (AssetId, File));

         String targetFile = AssetId + "-" + File;
         targetFile = format_path (_state.cacheDir + targetFile);
         
         if (is_valid_path (targetFile) && (ETag == asset->revision)) {
         
            StringContainer container;
            container.append (targetFile);

            _handle_reply (requestId, ForgeTypeGetAssetMedia, container);
         }
         else {
            
            DownloadStruct *ds = new DownloadStruct;
            ds->requestId = requestId;
            ds->requestType = ForgeTypeGetAssetMedia;
            ds->assetId = AssetId;
            ds->file = File;
            ds->hash = local_get_hash (File);
            ds->targetFile = targetFile;

            _state.downloadQueue.enqueue (ds);

            QTimer::singleShot (0, this, SLOT (_start_next_download ()));
         }
      }
      else { _handle_not_found (AssetId, requestId, ForgeTypeGetAssetMedia, observer); }
   }

   return requestId;
}


dmz::UInt64
dmz::ForgeModuleQt::add_asset_preview (
      const String &AssetId,
      const StringContainer &Files,
      ForgeObserver *observer) {

   UInt64 requestId (0);

   if (observer) {
      
      requestId = _state.requestCounter++;
      
      AssetStruct *asset = _state.assetTable.lookup (AssetId);
      if (asset) {

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
   //    AssetStruct *asset = _state.assetTable.lookup (AssetId);
   //    if (asset) {
   // 
   //       asset->preview.append (Preview);
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
dmz::ForgeModuleQt::_reply_finished () {
   
   QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender ());
   if (reply) {

      const Int32 StatusCode =
         reply->attribute (QNetworkRequest::HttpStatusCodeAttribute).toInt();

      QNetworkRequest request = reply->request ();
      
      const Int32 RequestType = request.attribute (LocalAttrType).toInt ();
      const UInt64 RequestId = request.attribute (LocalAttrId).toULongLong ();

      QByteArray data (reply->readAll ());
      const String JsonData (data.constData ());
      
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
// _state.log.warn << "<-- ForgeTypeSearch" << endl;
               _handle_search (RequestId, JsonData);
               break;
         
            case ForgeTypeGetAsset:
// _state.log.warn << "<-- ForgeTypeGetAsset" << endl;
               _handle_get_asset (RequestId, JsonData);
               break;
            
            case ForgeTypePutAsset:
// _state.log.warn << "<-- ForgeTypePutAsset" << endl;
                  _handle_put_asset (RequestId, JsonData);
               break;
         
            case ForgeTypeDeleteAsset:
// _state.log.warn << "<-- ForgeTypePutAsset" << endl;
               _handle_delete_asset (RequestId, JsonData);
               break;
         
            case LocalPutAssetMediaPhase1:
// _state.log.warn << "<-- LocalPutAssetMediaPhase1" << endl;
               _handle_put_asset_media_phase1 (RequestId, JsonData);
               break;

            case LocalPutAssetMediaPhase2:
// _state.log.warn << "<-- LocalPutAssetMediaPhase2" << endl;
               _handle_put_asset_media_phase2 (RequestId, JsonData);
               break;

            case LocalPutAssetMediaPhase3:
// _state.log.warn << "<-- LocalPutAssetMediaPhase3" << endl;
               _handle_put_asset_media_phase3 (RequestId, JsonData);
               break;
         
            case LocalAddAssetPreviewPhase1:
// _state.log.warn << "<-- LocalAddAssetPreviewPhase1" << endl;
               _handle_add_asset_preview_phase1 (RequestId, JsonData);
               break;
         
            case LocalAddAssetPreviewPhase2:
// _state.log.warn << "<-- LocalAddAssetPreviewPhase2" << endl;
               _handle_add_asset_preview_phase2 (RequestId, JsonData);
               break;

            case LocalAddAssetPreviewPhase3:
// _state.log.warn << "<-- LocalAddAssetPreviewPhase3" << endl;
               _handle_add_asset_preview_phase3 (RequestId, JsonData);
               break;

            case LocalGetUuids:
// _state.log.warn << "<-- LocalGetUuids" << endl;
               _handle_get_uuids (RequestId, JsonData);
               break;
         
            default:
               String msg ("Unknown request type: ");
               msg << RequestType;
            
               _state.log.warn << msg << endl;   

               _handle_error (RequestId, RequestType, msg);
         }
      }
      
      reply->deleteLater ();
   }
}


void
dmz::ForgeModuleQt::_download_progress (qint64 bytesReceived, qint64 bytesTotal) {
   
}


void
dmz::ForgeModuleQt::_download_ready_read () {

   if (_state.downloadReply && _state.downloadFile) {
      
      _state.downloadFile->write (_state.downloadReply->readAll ());
   }   
}


void
dmz::ForgeModuleQt::_download_finished () {

   if (_state.download && _state.downloadReply && _state.downloadFile) {
   
      const Int32 StatusCode =
         _state.downloadReply->attribute (QNetworkRequest::HttpStatusCodeAttribute).toInt();

      QNetworkRequest request = _state.downloadReply->request ();
      
      const Int32 RequestType = request.attribute (LocalAttrType).toInt ();
      const UInt64 RequestId = request.attribute (LocalAttrId).toULongLong ();

      Boolean validDownload (False);
      
      if (_state.downloadReply->error () == QNetworkReply::NoError) {

         validDownload = True;
         
         _state.downloadFile->close ();

         if (_state.download->hash) {
            
            const QString FileName (_state.downloadFile->fileName ());
            const String Hash (sha_from_file (qPrintable (FileName)));

            if (Hash == _state.download->hash) {
               
               _state.log.info << "Downloaded file verified: "
                               << _state.download->assetId << "/"
                               << _state.download->file << endl;
            }
            else {
               
               validDownload = False;
               
               _state.log.warn << "Invalid hash for download: "
                               << _state.download->assetId << "/"
                               << _state.download->file << endl;
            }
         }
      }
      else {
         
         String msg ("Network Error: ");
         msg << qPrintable (_state.downloadReply->errorString ());
         
         _handle_error (RequestId, RequestType, msg);
      }
      
      if (validDownload) {
         
         _state.downloadFile->setAutoRemove (False);
         
         if (is_valid_path (_state.download->targetFile)) { 
            
            remove_file (_state.download->targetFile);
         }
         else { _state.store_etag (_state.download->assetId, _state.download->file); }
         
         if (_state.downloadFile->rename (_state.download->targetFile.get_buffer ())) {

            _state.update_etag (_state.download->assetId, _state.download->file);
            
            StringContainer container;
            container.append (_state.download->targetFile);

            _handle_reply (RequestId, RequestType, container);
         }
         else {
 
            _state.downloadFile->setAutoRemove (True);

            String msg ("Failed to rename downloaded file for: ");
            msg << _state.download->assetId << "/" << _state.download->file;

            _handle_error (RequestId, RequestType, msg);
         }
      }
      
      delete _state.downloadFile;
      _state.downloadFile = 0;
      
      delete _state.download;
      _state.download = 0;
      
      _state.downloadReply->deleteLater ();
      _state.downloadReply = 0;
      
      _start_next_download ();
   }
}


void
dmz::ForgeModuleQt::_upload_progress (qint64 bytesSent, qint64 bytesTotal) {

//    if (_state.uploadReply) {
//       
//       QNetworkRequest request = _state.uploadReply->request ();
//       const UInt64 Id = request.attribute (LocalAttrId).toULongLong ();
// _state.log.error << "_upload_progress["  << Id  << "]: " << bytesSent << " of " << bytesTotal << endl;
//    }
}


void
dmz::ForgeModuleQt::_upload_finished () {

   if (_state.uploadReply) {
      
      // QNetworkRequest request = _state.uploadReply->request ();
      // const UInt64 Id = request.attribute (LocalAttrId).toULongLong ();
      
      _state.uploadReply->deleteLater ();
      _state.uploadReply = 0;
   }
   
   if (_state.uploadFile) {
      
      _state.uploadFile->close ();
      _state.uploadFile->deleteLater ();
      _state.uploadFile = 0;
   }

   if (_state.upload && (_state.upload->files.get_count () == 0)) { _state.upload = 0; }
   
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
            request.setRawHeader (LocalUserAgent, LocalUserAgentName);
            request.setRawHeader (LocalAccept, LocalApplicationJson);
            request.setHeader (QNetworkRequest::ContentTypeHeader, mimeType);
            request.setAttribute (LocalAttrId, _state.upload->requestId);
            request.setAttribute (LocalAttrType, (int)_state.upload->requestType);

            _state.uploadFile = new QFile (fi.absoluteFilePath ());

            if (_state.uploadFile->open (QIODevice::ReadOnly)) {

               _state.uploadReply =
                  _state.networkAccessManager->put (request, _state.uploadFile);
            
               connect (
                  _state.uploadReply, SIGNAL (finished ()),
                  this, SLOT (_reply_finished ()));
            
               connect (
                  _state.uploadReply, SIGNAL (uploadProgress (qint64, qint64)),
                  this, SLOT (_upload_progress (qint64, qint64)));
               
               connect (
                  _state.uploadReply, SIGNAL (finished ()),
                  this, SLOT (_upload_finished ()));
            }
            else {

               _state.log.warn << "Failed to open file: "
                               << qPrintable (fi.absoluteFilePath ()) << endl;
                               
               delete _state.uploadFile;
               _state.uploadFile = 0;
            }
         } 
      }
   }
}


void
dmz::ForgeModuleQt::_start_next_download () {

   // only download next file if not currently downloading
   if (!_state.download && !_state.downloadFile && !_state.downloadReply) {

      // get next file  off of the queue
      if (!_state.downloadQueue.isEmpty ()) { 

         _state.download = _state.downloadQueue.dequeue ();
      }
      
      if (_state.download && _state.networkAccessManager) {

         _state.downloadFile = new QTemporaryFile (QDir::tempPath () + "/dmz_forge_download.XXXXXX");
         if (_state.downloadFile->open ()) {
      
            QUrl url (_state.baseUrl);
            QString path ("/%1/%2/%3");
         
            url.setPath (
               path.arg (_state.db)
                   .arg (_state.download->assetId.get_buffer ())
                   .arg (_state.download->file.get_buffer ()));

            QNetworkRequest request (url);
            request.setRawHeader (LocalUserAgent, LocalUserAgent);
            // request.setRawHeader (LocalAccept, LocalApplicationJson);
            // request.setHeader (QNetworkRequest::ContentTypeHeader, mimeType);
            request.setAttribute (LocalAttrId, _state.download->requestId);
            request.setAttribute (LocalAttrType, (int)_state.download->requestType);

            _state.downloadReply = _state.networkAccessManager->get (request);
      
            connect (
               _state.downloadReply, SIGNAL (downloadProgress (qint64, qint64)),
               this, SLOT (_download_progress (qint64, qint64)));
      
            connect(
               _state.downloadReply, SIGNAL (readyRead ()),
               this, SLOT (_download_ready_read ()));
            
            connect (
               _state.downloadReply, SIGNAL (finished ()),
               this, SLOT (_download_finished ()));
         }
         else {

            _state.log.warn << "Failed to download file: " 
                            << _state.download->assetId << "/"
                            << _state.download->file << endl;
                            
            delete _state.downloadFile;
            _state.downloadFile = 0;
            
            delete _state.download;
            _state.download = 0;
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

      const String AssetId (config_to_string (Local_Id, global));

      AssetStruct *asset = _state.assetTable.lookup (AssetId);
      if (asset){

         // _config_to_asset will overwrite type, save it so we don't loose it
         String type = asset->type;
         QMap<QString, String>current = asset->current;
         QMap<QString, String>originalName = asset->originalName;
         
         _config_to_asset (global);

         // now lets restore type and push to server
         asset->type = type;
         asset->current = current;
         asset->originalName = originalName;
         
         _put_asset (AssetId, RequestId, LocalPutAssetMediaPhase3);
      }
   }
   else {
      
      _handle_error (RequestId, ForgeTypePutAssetMedia, LocalJsonParseErrorMessage);
   }
}


void
dmz::ForgeModuleQt::_handle_put_asset_media_phase3 (
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

         _handle_reply (RequestId, ForgeTypePutAssetMedia, container);
      }
      else {

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

      AssetStruct *asset = _state.assetTable.lookup (AssetId);
      if (asset){

         // _config_to_asset will overwrite previews, save it so we don't loose it
         StringContainer previews = asset->previews;
         _config_to_asset (global);

         // now lets restore previews and push to server
         asset->previews = previews;
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
dmz::ForgeModuleQt::_request (
      const QString &Method,
      const QUrl &Url,
      const UInt64 RequestId,
      const Int32 RequestType,
      const QByteArray &Data) {

   QNetworkReply *reply (0);
   
   if (_state.networkAccessManager) {
      
      QNetworkRequest request (Url);
      
      request.setRawHeader (LocalUserAgent, LocalUserAgent);
      request.setRawHeader (LocalAccept, LocalApplicationJson);
      
      request.setAttribute (LocalAttrId, RequestId);
      request.setAttribute (LocalAttrType, (int)RequestType);

      if (LocalGet == Method.toLower ()) {
         
         reply = _state.networkAccessManager->get (request);
      }
      else if (LocalPut == Method.toLower ()) {
         
         reply = _state.networkAccessManager->put (request, Data);
      }
      else if (LocalDelete == Method.toLower ()) {
         
         reply = _state.networkAccessManager->deleteResource (request);
      }
      else {
         
         _state.log.warn << "Unknown HTTP method requested: " << qPrintable (Method) << endl;
         _state.log.warn << "with url: " << qPrintable (Url.toString ()) << endl;
      }
      
      if (reply) {
         
         connect (reply, SIGNAL (finished ()), this, SLOT (_reply_finished ()));
      }
   }
   
   return reply;
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

      reply = _request (LocalGet, url, RequestId, RequestType);
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

      reply = _request (LocalPut, url, RequestId, RequestType, byteArray);
   }
   
   return reply;
}


QString
dmz::ForgeModuleQt::_add_asset_media_file (const String &AssetId, const String &File) {

   QFileInfo fi (File.get_buffer ());

   const QString FileSHA = sha_from_file (File).get_buffer ();
   
   QString name (FileSHA);
   
   AssetStruct *asset = _state.assetTable.lookup (AssetId);
   if (asset) {
      
      const QString Ext (fi.suffix ().toLower ());

      QByteArray mimeType (LocalMimeIVE);
      if (localMimeTypes.contains (Ext)) { mimeType = localMimeTypes[Ext]; }
      
      const QString Rev (local_get_current_rev (asset->current.value (mimeType)));
      
      name = Rev + "-" + FileSHA;
      asset->originalName[name] = qPrintable (fi.fileName ());

      name = name + "." + Ext;
      asset->current[mimeType] = qPrintable (name);

      if (Ext == QLatin1String ("ive")) {
         
         asset->type = LocalMedia3d;
      }
      else if (Ext == QLatin1String ("wav")) {
         
         asset->type = LocalMediaAudio;
      }
      else if (Ext == QLatin1String ("xml") || Ext == QLatin1String ("json")) {
         
         asset->type = LocalMediaConfig;
      }
      else if (Ext == QLatin1String ("jpg") || Ext == QLatin1String ("png")) {
         
         asset->type = LocalMediaImage;
      }
      else { asset->type = "unknown"; }
   }
   
   return name;
}



QString
dmz::ForgeModuleQt::_add_asset_preview_file (const String &AssetId, const String &File) {

   String path, name, ext;
   split_path_file_ext (File, path, name, ext);

   const String Preview (name + ext);
   
   AssetStruct *asset = _state.assetTable.lookup (AssetId);
   if (asset) { asset->previews.append (Preview); }
   
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
   
   AssetStruct *asset = _state.assetTable.lookup (AssetId);
   if (asset) {
      
      assetConfig.store_attribute (Local_Id, asset->Id);

      if (asset->revision) { assetConfig.store_attribute (Local_Rev, asset->revision); }

      assetConfig.store_attribute (LocalType, "asset");
      assetConfig.store_attribute (LocalName, asset->name);
      assetConfig.store_attribute (LocalBrief, asset->brief);
      assetConfig.store_attribute (LocalDetails, asset->details);
      assetConfig.store_attribute (LocalMedia, asset->type);
      
      // keywords
      String keyword;
      Boolean first = True;
      StringContainerIterator keywordIt;
      
      while (asset->keywords.get_next (keywordIt, keyword)) {
      
         Config data (LocalKeywords);
         data.store_attribute (LocalValue, keyword);
         
         if (first) { assetConfig.overwrite_config (data); first = False; }
         else { assetConfig.add_config (data); }
      }

      // current
      Config currentConfig (LocalCurrent);
      QMapIterator<QString, String> currentIt (asset->current);
      while (currentIt.hasNext()) {
      
         currentIt.next();
         currentConfig.store_attribute (qPrintable (currentIt.key ()), currentIt.value ());
      }
      
      assetConfig.overwrite_config (currentConfig);
      
      // originalName
      Config nameConfig (LocalOriginalName);
      QMapIterator<QString, String> nameIt (asset->originalName);
      while (nameIt.hasNext()) {
      
         nameIt.next();
         nameConfig.store_attribute (qPrintable (nameIt.key ()), nameIt.value ());
      }
      
      assetConfig.overwrite_config (nameConfig);
      
      // previews
      Config previewsConfig (LocalPreviews);
      StringContainerIterator previewIt;
      String image;
      
      while (asset->previews.get_next (previewIt, image)) {
      
         Config data (LocalImages);
         data.store_attribute (LocalValue, image);
         
         previewsConfig.add_config (data);
      }
      
      assetConfig.overwrite_config (previewsConfig);
      
      // _attachments
      assetConfig.overwrite_config (asset->attachments);
      
      retVal = True;
   }
   
   return retVal;
}


dmz::Boolean
dmz::ForgeModuleQt::_config_to_asset (const Config &AssetConfig) {

   Boolean retVal (False);
   
   const String AssetId (config_to_string (Local_Id, AssetConfig));
   
   AssetStruct *asset = _state.assetTable.lookup (AssetId);
   if (!asset && AssetId) {
      
      create_asset (AssetId);
      asset = _state.assetTable.lookup (AssetId);
   }
   
   if (asset) {
      
      _store_revision (AssetId, config_to_string (Local_Rev, AssetConfig));
      
      store_name (AssetId, config_to_string (LocalName, AssetConfig));
      store_brief (AssetId, config_to_string (LocalBrief, AssetConfig));
      store_details (AssetId, config_to_string (LocalDetails, AssetConfig));
      
      asset->type = config_to_string (LocalMedia, AssetConfig);
      
      // keywords
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

      // current
      asset->current.clear ();
      Config currentConfig;
      
      if (AssetConfig.lookup_config (LocalCurrent, currentConfig)) {
         
         ConfigIterator it;
         Config data;
         
         while (currentConfig.get_next_config (it, data)) {
            
            QString mimeType (data.get_name ().get_buffer ());
            
            asset->current.insert (mimeType, config_to_string (LocalValue, data));
         }
      }

      // originalName
      asset->originalName.clear ();
      Config nameConfig;
      
      if (AssetConfig.lookup_config (LocalCurrent, nameConfig)) {
         
         ConfigIterator it;
         Config data;
         
         while (nameConfig.get_next_config (it, data)) {
            
            QString name (data.get_name ().get_buffer ());
            
            asset->originalName.insert (name, config_to_string (LocalValue, data));
         }
      }
      
      // previews
      asset->previews.clear ();
      Config previewsConfig;
      
      if (AssetConfig.lookup_config (LocalPreviews, previewsConfig)) {
         
         Config imageList;

         if (previewsConfig.lookup_all_config (LocalImages, imageList)) {

            ConfigIterator it;
            Config image;

            while (imageList.get_next_config (it, image)) {

               asset->previews.append (config_to_string (LocalValue, image));
            }
         }
      }
      
      // _attachments
      AssetConfig.lookup_config (Local_Attachments, asset->attachments);
      
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

         QNetworkReply *reply = _request (LocalGet, url, requestId, LocalGetUuids);
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
