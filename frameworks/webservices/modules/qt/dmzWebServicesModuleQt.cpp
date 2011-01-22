#include <dmzArchiveModule.h>
#include <dmzFoundationJSONUtil.h>
#include <dmzObjectAttributeMasks.h>
#include <dmzObjectModule.h>
#include "dmzQtHttpClient.h"
#include <dmzRuntimeConfig.h>
#include <dmzRuntimeConfigToTypesBase.h>
#include <dmzRuntimeDefinitions.h>
#include <dmzRuntimePluginFactoryLinkSymbol.h>
#include <dmzRuntimePluginInfo.h>
#include <dmzRuntimeLog.h>
#include <dmzRuntimeObjectType.h>
#include <dmzTypesHandleContainer.h>
#include <dmzTypesString.h>
#include <dmzTypesStringContainer.h>
#include <dmzTypesUUID.h>
#include <dmzSystemStreamString.h>
#include <dmzWebServicesConsts.h>
#include "dmzWebServicesModuleQt.h"
#include <QtCore/QtCore>
#include <QtNetwork/QtNetwork>

using namespace dmz;


namespace {
   
   // static const char LocalDatabaseEndpoint[] = "http://dmz.couchone.com:80";
   static const char LocalDatabaseEndpoint[] = "http://localhost:5984";
   static const char LocalUserAgentName[] = "dmzWebServicesModuleQt";
   
   static const String LocalJsonParseErrorMessage ("Error parsing json response.");
   
   static const String Local_Id ("_id");
   static const String Local_Rev ("_rev");
   static const String Local_Attachments ("_attachments");
   
   static const QString LocalGet ("get");
   static const QString LocalPut ("put");
   static const QString LocalDelete ("delete");
   static const QString LocalTimeStampFormat ("yyyy-M-d-h-m-s");

   static const char LocalId[] = "id";
   static const char LocalRev[] = "rev";
   static char LocalAccept[] = "Accept";
   static char LocalApplicationJson[] = "application/json";
   static char LocalApplicationOctetStream[] = "application/octet-stream";
   static char LocalUserAgent[] = "User-Agent";
   static char LocalETag[]= "ETag";
   static char LocalIfNoneMatch[] = "If-None-Match";

   static QNetworkRequest::Attribute LocalAttrType =
      (QNetworkRequest::Attribute) (QNetworkRequest::User + 1);

   static QNetworkRequest::Attribute LocalAttrId =
      (QNetworkRequest::Attribute) (QNetworkRequest::User + 2);
};


struct dmz::WebServicesModuleQt::State {

   Log log;
   QtHttpClient *client;
   QUrl baseUrl;
   // HashTableUInt64Template<ForgeObserver> obsTable;
   QMap<UInt64, QString> requestMap;
   ArchiveModule *archiveMod;
   Handle archiveHandle;
   Handle revisionAttrHandle;
   Handle publishAttrHandle;
   ObjectTypeSet includeSet;
   HandleContainer includeTable;
   HandleContainer updateTable;
   QMap<QString, String> revisionMap;
   
   State (const PluginInfo &Info);
   ~State ();
};


dmz::WebServicesModuleQt::State::State (const PluginInfo &Info) :
      log (Info),
      client (0),
      baseUrl (LocalDatabaseEndpoint),
      archiveMod (0),
      archiveHandle (0) {

}


dmz::WebServicesModuleQt::State::~State () {

   if (client) {

      client->deleteLater ();
      client = 0;
   }
}


dmz::WebServicesModuleQt::WebServicesModuleQt (const PluginInfo &Info, Config &local) :
      QObject (0),
      Plugin (Info),
      TimeSlice (Info),
      ObjectObserverUtil (Info, local),
      WebServicesModule (Info),
      _state (*(new State (Info))) {

   _state.client = new QtHttpClient (Info, this);
   _state.client->update_username ("admin", "couch4me");

   connect (
      _state.client, SIGNAL (reply_finished (const UInt64, QNetworkReply *)),
      this, SLOT (_reply_finished (const UInt64, QNetworkReply *)));

   connect (
      _state.client, SIGNAL (reply_aborted (const UInt64)),
      this, SLOT (_reply_aborted (const UInt64)));

   // connect (
   //    _state.client, SIGNAL (
   //       reply_error (
   //          const UInt64,
   //          const QString &,
   //          const QNetworkReply::NetworkError)),
   //    this, SLOT (
   //       _reply_error (
   //          const UInt64,
   //          const QString &,
   //          const QNetworkReply::NetworkError)));
      
   stop_time_slice ();
   
   _init (local);
}


dmz::WebServicesModuleQt::~WebServicesModuleQt () {

   delete &_state;
}


// Plugin Interface
void
dmz::WebServicesModuleQt::update_plugin_state (
      const PluginStateEnum State,
      const UInt32 Level) {

   if (State == PluginStateInit) {

   }
   else if (State == PluginStateStart) {

      // QUrl url (_state.baseUrl);
      // url.setPath ("/test");
      // 
      // UInt64 requestId = _state.client->get (url);
   }
   else if (State == PluginStateStop) {

   }
   else if (State == PluginStateShutdown) {

   }
}


void
dmz::WebServicesModuleQt::discover_plugin (
      const PluginDiscoverEnum Mode,
      const Plugin *PluginPtr) {

   if (Mode == PluginDiscoverAdd) {

      if (!_state.archiveMod) { _state.archiveMod = ArchiveModule::cast (PluginPtr); }
   }
   else if (Mode == PluginDiscoverRemove) {

      if (_state.archiveMod && (_state.archiveMod == ArchiveModule::cast (PluginPtr))) {

         _state.archiveMod = 0;
      }
   }
}


// TimeSlice Interface
void
dmz::WebServicesModuleQt::update_time_slice (const Float64 TimeDelta) {

   ObjectModule *objMod (get_object_module ());
   // if (objMod) {
   //    
   //    HandleContainerIterator it;
   //    Handle next;
   // 
   //    while (_state.updateTable.get_next (it, next)) {
   // 
   //       UUID uuid;
   //       if (objMod->lookup_uuid (next, uuid)) {
   //          
   //          put_object (next);
   //       }
   // 
   //       _state.updateTable.remove (next);
   //    }
   // }
   

   _state.updateTable.clear ();
   if (_state.updateTable.get_count () == 0) { stop_time_slice (); }

   if (_state.archiveMod && objMod) {
      
      Config archive = _state.archiveMod->create_archive (_state.archiveHandle);
      Config objectList;
      
      if (archive.lookup_all_config ("archive.object", objectList)) {
         
         ConfigIterator it;
         Config object;

         while (objectList.get_next_config (it, object)) {
         
            _state.log.error << "------------------------------------" << endl;
            UUID uuid = config_to_string ("uuid", object);
            
            String id = uuid.to_string ();
            String rev = _state.revisionMap.value (id.get_buffer ());

            Config doc ("global");
            
            doc.store_attribute ("_id", id);
            
            if (rev) { doc.store_attribute ("_rev", rev); }
            
            doc.add_config (object);
            
            String json;
            StreamString out (json);
                           
            if (format_config_to_json (doc, out, ConfigStripGlobal|ConfigPrettyPrint, &_state.log)) {
            
               QUrl url (_state.baseUrl);
               url.setPath (QString ("/test/%1").arg (id.get_buffer ()));
            
               QByteArray data (json.get_buffer ());
            
               const UInt64 RequestId (_state.client->put (url, data));

               _state.log.warn << "put: " << RequestId << endl;
            }
            
            _state.log.error << "------------------------------------" << endl;
         }
      }
         
         // QUrl url (_state.baseUrl);
         // url.setPath (QString ("/test/%1").arg (next));
         // 
         // QByteArray data (json.get_buffer ());
         // 
         // const UInt64 RequestId (_state.client->put (url, data));
         
         // _state.requestMap.insert (RequestId, QLatin1String ("put_object"));
      // }
   }

   // while (!_state.replyQueue.isEmpty ()) {
   // 
   //    ReplyStruct *rs = _state.replyQueue.dequeue ();
   // 
   //    if (_state.replyQueue.isEmpty ()) { stop_time_slice (); }
   // 
   //    ForgeObserver *observer = _state.obsTable.remove (rs->RequestId);
   //    if (observer) {
   // 
   //       observer->handle_reply (
   //          rs->RequestId,
   //          rs->RequestType,
   //          rs->Error,
   //          rs->Container);
   //    }
   // 
   //    delete rs; rs = 0;
   // }
}


// Object Observer Interface
void
dmz::WebServicesModuleQt::create_object (
      const UUID &Identity,
      const Handle ObjectHandle,
      const ObjectType &Type,
      const ObjectLocalityEnum Locality) {

   if (_state.includeSet.contains_type (Type)) {
   
      _state.includeTable.add (ObjectHandle);
      _state.updateTable.add (ObjectHandle);
   }
   
   // if (_state.archiveMod) {
   // 
   //    Config config = _state.archiveMod->create_archive (_state.archiveHandle);
   // 
   //    String data;
   //    StreamString out (data);
   // 
   //    if (format_config_to_json (config, out, ConfigStripGlobal|ConfigPrettyPrint, &_state.log)) {
   // 
   //       _state.log.warn << data << endl;
   //    }
   // }
}


void
dmz::WebServicesModuleQt::destroy_object (
      const UUID &Identity,
      const Handle ObjectHandle) {

   if (_state.includeTable.contains (ObjectHandle)) {
      
      _state.includeTable.remove (ObjectHandle);
   }
}


void
dmz::WebServicesModuleQt::update_object_uuid (
      const Handle ObjectHandle,
      const UUID &Identity,
      const UUID &PrevIdentity) {

   _add_update (ObjectHandle);
}


void
dmz::WebServicesModuleQt::remove_object_attribute (
      const UUID &Identity,
      const Handle ObjectHandle,
      const Handle AttributeHandle,
      const Mask &AttrMask) {

   _add_update (ObjectHandle);
}


void
dmz::WebServicesModuleQt::update_object_locality (
      const UUID &Identity,
      const Handle ObjectHandle,
      const ObjectLocalityEnum Locality,
      const ObjectLocalityEnum PrevLocality) {

}


void
dmz::WebServicesModuleQt::link_objects (
      const Handle LinkHandle,
      const Handle AttributeHandle,
      const UUID &SuperIdentity,
      const Handle SuperHandle,
      const UUID &SubIdentity,
      const Handle SubHandle) {

   // _add_update (SuperHandle);
}


void
dmz::WebServicesModuleQt::unlink_objects (
      const Handle LinkHandle,
      const Handle AttributeHandle,
      const UUID &SuperIdentity,
      const Handle SuperHandle,
      const UUID &SubIdentity,
      const Handle SubHandle) {

   // _add_update (SuperHandle);
}


void
dmz::WebServicesModuleQt::update_link_attribute_object (
      const Handle LinkHandle,
      const Handle AttributeHandle,
      const UUID &SuperIdentity,
      const Handle SuperHandle,
      const UUID &SubIdentity,
      const Handle SubHandle,
      const UUID &AttributeIdentity,
      const Handle AttributeObjectHandle,
      const UUID &PrevAttributeIdentity,
      const Handle PrevAttributeObjectHandle) {

   // _add_update (SuperHandle);
}


void
dmz::WebServicesModuleQt::update_object_counter (
      const UUID &Identity,
      const Handle ObjectHandle,
      const Handle AttributeHandle,
      const Int64 Value,
      const Int64 *PreviousValue) {

   _add_update (ObjectHandle);
}


void
dmz::WebServicesModuleQt::update_object_counter_minimum (
      const UUID &Identity,
      const Handle ObjectHandle,
      const Handle AttributeHandle,
      const Int64 Value,
      const Int64 *PreviousValue) {

   _add_update (ObjectHandle);
}


void
dmz::WebServicesModuleQt::update_object_counter_maximum (
      const UUID &Identity,
      const Handle ObjectHandle,
      const Handle AttributeHandle,
      const Int64 Value,
      const Int64 *PreviousValue) {

   _add_update (ObjectHandle);
}


void
dmz::WebServicesModuleQt::update_object_alternate_type (
      const UUID &Identity,
      const Handle ObjectHandle,
      const Handle AttributeHandle,
      const ObjectType &Value,
      const ObjectType *PreviousValue) {

   _add_update (ObjectHandle);
}


void
dmz::WebServicesModuleQt::update_object_state (
      const UUID &Identity,
      const Handle ObjectHandle,
      const Handle AttributeHandle,
      const Mask &Value,
      const Mask *PreviousValue) {

   _add_update (ObjectHandle);
}


void
dmz::WebServicesModuleQt::update_object_flag (
      const UUID &Identity,
      const Handle ObjectHandle,
      const Handle AttributeHandle,
      const Boolean Value,
      const Boolean *PreviousValue) {

   _add_update (ObjectHandle);
}


void
dmz::WebServicesModuleQt::update_object_time_stamp (
      const UUID &Identity,
      const Handle ObjectHandle,
      const Handle AttributeHandle,
      const Float64 Value,
      const Float64 *PreviousValue) {

   _add_update (ObjectHandle);
}


void
dmz::WebServicesModuleQt::update_object_position (
      const UUID &Identity,
      const Handle ObjectHandle,
      const Handle AttributeHandle,
      const Vector &Value,
      const Vector *PreviousValue) {

   _add_update (ObjectHandle);
}


void
dmz::WebServicesModuleQt::update_object_orientation (
      const UUID &Identity,
      const Handle ObjectHandle,
      const Handle AttributeHandle,
      const Matrix &Value,
      const Matrix *PreviousValue) {

   _add_update (ObjectHandle);
}


void
dmz::WebServicesModuleQt::update_object_velocity (
      const UUID &Identity,
      const Handle ObjectHandle,
      const Handle AttributeHandle,
      const Vector &Value,
      const Vector *PreviousValue) {

   _add_update (ObjectHandle);
}


void
dmz::WebServicesModuleQt::update_object_acceleration (
      const UUID &Identity,
      const Handle ObjectHandle,
      const Handle AttributeHandle,
      const Vector &Value,
      const Vector *PreviousValue) {

   _add_update (ObjectHandle);
}


void
dmz::WebServicesModuleQt::update_object_scale (
      const UUID &Identity,
      const Handle ObjectHandle,
      const Handle AttributeHandle,
      const Vector &Value,
      const Vector *PreviousValue) {

   _add_update (ObjectHandle);
}


void
dmz::WebServicesModuleQt::update_object_vector (
      const UUID &Identity,
      const Handle ObjectHandle,
      const Handle AttributeHandle,
      const Vector &Value,
      const Vector *PreviousValue) {

   _add_update (ObjectHandle);
}


void
dmz::WebServicesModuleQt::update_object_scalar (
      const UUID &Identity,
      const Handle ObjectHandle,
      const Handle AttributeHandle,
      const Float64 Value,
      const Float64 *PreviousValue) {

   _add_update (ObjectHandle);
}


void
dmz::WebServicesModuleQt::update_object_text (
      const UUID &Identity,
      const Handle ObjectHandle,
      const Handle AttributeHandle,
      const String &Value,
      const String *PreviousValue) {

   _add_update (ObjectHandle);
}


void
dmz::WebServicesModuleQt::update_object_data (
      const UUID &Identity,
      const Handle ObjectHandle,
      const Handle AttributeHandle,
      const Data &Value,
      const Data *PreviousValue) {

   _add_update (ObjectHandle);
}


// WebServicesModule Interface
dmz::UInt64
dmz::WebServicesModuleQt::put_object (
      const UUID &Identity,
      WebServicesObserver *observer) {

   // if (_state.archiveMod) {
   //    
   //    Config config = _state.archiveMod->create_archive (_state.archiveHandle);
   //    
   //    String data;
   //    StreamString out (data);
   // 
   //    if (format_config_to_json (config, out, ConfigStripGlobal, &_state.log)) {
   //    
   //       _state.log.warn << data << endl;
   //    }
   // }

#if 0
   ObjectModule *objMod (get_object_module ());
   if (objMod) {

      Config objConfig ("object");

      Boolean dumpObject (True);

      const ObjectType Type (objMod->lookup_object_type (ObjectHandle));

      if (Type) {

      }
      else { dumpObject = False; }
      
      if (dumpObject) {

         objConfig.store_attribute ("type", Type.get_name ());

         UUID uuid;

         if (objMod->lookup_uuid (ObjectHandle, uuid)) {

            objConfig.store_attribute ("uuid", uuid.to_string ());
         }

         // _currentConfig = result = tmp;

         objMod->dump_all_object_attributes (ObjectHandle, *this);
      }
   }
#endif

   return 0;
}


dmz::UInt64
dmz::WebServicesModuleQt::get_object (
      const UUID &Identity,
      WebServicesObserver *observer) {

   return 0;
}


void
dmz::WebServicesModuleQt::_reply_aborted (const UInt64 RequestId) {

   _state.log.warn << "_reply_aborted: " << RequestId << endl;
}


void
dmz::WebServicesModuleQt::_reply_finished (const UInt64 RequestId, QNetworkReply *reply) {

   if (reply) {
      
      const Int32 StatusCode =
         reply->attribute (QNetworkRequest::HttpStatusCodeAttribute).toInt();

      QByteArray data (reply->readAll ());
      const String JsonData (data.constData ());
      
      if (JsonData) {
         
         Config global ("global");
         if (json_string_to_config (JsonData, global)) {
      
            _handle_reply (RequestId, StatusCode, global);
         }
         else {
      
            String msg = "Error parsing json response";
            _state.log.error << msg << endl;
            // _handle_error (RequestId, StatusCode, msg);
         }
      }
      else {
         
         String msg ("Network Error: ");
         msg << qPrintable (reply->errorString ());
         
         _state.log.error << msg << endl;
         // _handle_error (RequestId, StatusCode, msg);
      }
   }
}


void
dmz::WebServicesModuleQt::_handle_reply (
      const UInt64 RequestId,
      const Int32 StatusCode,
      const Config &Global) {

_state.log.warn << "_reply_finished[" << RequestId << "]: " << StatusCode << endl;

   String error = config_to_string ("error", Global);
   if (error) {

      String msg = config_to_string ("reason", Global);
      _state.log.error << msg << endl;
   }
   else {

      String id = config_to_string ("id", Global);
      String rev = config_to_string ("rev", Global);

      if (id && rev) {
      
         _state.revisionMap.insert (id.get_buffer (), rev);
      }
      
      _state.log.warn << " id: " << id << endl;
      _state.log.warn << "rev: " << rev << endl;
   }
}


// void
// dmz::WebServicesModuleQt::_reply_error (
//       const UInt64 RequestId,
//       const QString &ErrorMessage,
//       const QNetworkReply::NetworkError Error) {
//          
//    _state.log.error << "_reply_error[" << RequestId << "]: "
//       << (Int32)Error << " - " << qPrintable (ErrorMessage) << endl;    
// }


// WebServicesModuleQt Interface
void
dmz::WebServicesModuleQt::_add_update (const Handle &ObjectHandle) {
   
   if (!_state.includeTable.contains (ObjectHandle)) {

      _state.updateTable.add (ObjectHandle);
      
      if (_state.updateTable.get_count () == 1) { start_time_slice (); }
   }
}


void
dmz::WebServicesModuleQt::_init (Config &local) {

   setObjectName (get_plugin_name ().get_buffer ());
   
   Definitions defs (get_plugin_runtime_context ());
   RuntimeContext *context (get_plugin_runtime_context ());

   _state.archiveHandle = defs.create_named_handle (
      config_to_string ("archive.name", local, WebServicesArchiveName));
   
   _state.publishAttrHandle = activate_object_attribute (
      WebServicesPublishAttributeName,
      ObjectFlagMask);

   _state.revisionAttrHandle = defs.create_named_handle (WebServicesRevisionAttributeName);
            
   Config typeList;
   if (local.lookup_all_config ("include.type", typeList)) {

      ConfigIterator it;
      Config type;

      while (typeList.get_next_config (it, type)) {

         _state.includeSet.add_object_type (config_to_string ("name", type), context);
      }
   }
   
   Config webservice;
   if (local.lookup_config ("webservice", webservice)) {

      // Config proxy;
      // if (webservice.lookup_config ("proxy", proxy)) {
      // 
      //    String host = config_to_string ("host", proxy, "localhost");
      //    Int32 port = config_to_int32 ("port", proxy, 8888);
      // 
      //    QNetworkProxy proxy;
      //    proxy.setType (QNetworkProxy::HttpProxy);
      //    proxy.setHostName (host.get_buffer ());
      //    proxy.setPort (port);
      // 
      //    QNetworkProxy::setApplicationProxy(proxy);
      // 
      //    _state.log.info << "Using proxy: " << host << ":" << port << endl;
      // }
      // 
      // String host = qPrintable (_state.baseUrl.host ());
      // host =  config_to_string ("host", webservice, host);
      // _state.baseUrl.setHost (host.get_buffer ());
      // 
      // Int32 port = _state.baseUrl.port ();
      // port = config_to_int32 ("port", webservice, port);
      // _state.baseUrl.setPort (port);
      // 
      // _state.log.info
      //    << "Using API endpoint: " << qPrintable (_state.baseUrl.toString ()) << endl;
      // 
      // _state.db = config_to_string ("db", webservice, "assets").get_buffer ();
   }
   
   // activate_default_object_attribute (ObjectCreateMask | ObjectDestroyMask);
   activate_global_object_observer ();
}


extern "C" {

DMZ_PLUGIN_FACTORY_LINK_SYMBOL dmz::Plugin *
create_dmzWebServicesModuleQt (
      const dmz::PluginInfo &Info,
      dmz::Config &local,
      dmz::Config &global) {

   return new dmz::WebServicesModuleQt (Info, local);
}

};
