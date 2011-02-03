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
#include <dmzTypesHashTableHandleTemplate.h>
#include <dmzTypesHashTableUInt64Template.h>
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
//   static const char LocalDatabaseEndpoint[] = "http://localhost:5984";
   static const char LocalDatabaseEndpoint[] = "https://games.chds.us:443";
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

   static const Float64 LocalFetchChangesInterval (30.0);

   struct PendingStruct {

      String docId;
      UInt64 requestId;
      String requestType;
      Handle objectHandle;

      PendingStruct () : requestId (0), objectHandle (0) {;}
   };
};


struct dmz::WebServicesModuleQt::State {

   Log log;
   QtHttpClient *client;
   QUrl baseUrl;
   // HashTableUInt64Template<ForgeObserver> obsTable;
   ArchiveModule *archiveMod;
   Handle archiveHandle;
   Handle revisionAttrHandle;
   Handle publishAttrHandle;
   ObjectTypeSet includeSet;
   HandleContainer includeTable;
   HandleContainer updateTable;
   QMap<QString, String> revisionMap;
   HashTableUInt64Template<PendingStruct> pendingTable;
   StringContainer fetchTable;
   Int32 lastSeq;
   Float64 fetchChangesDelta;

   State (const PluginInfo &Info);
   ~State ();

   PendingStruct *get_pending (const UInt64 RequestId);
   Boolean is_object_pending (const Handle ObjectHandle);
};


dmz::WebServicesModuleQt::State::State (const PluginInfo &Info) :
      log (Info),
      client (0),
      baseUrl (LocalDatabaseEndpoint),
      archiveMod (0),
      archiveHandle (0),
      fetchChangesDelta (0),
      lastSeq (0) {

}


dmz::WebServicesModuleQt::State::~State () {

   pendingTable.empty ();

   if (client) {

      client->deleteLater ();
      client = 0;
   }
}


PendingStruct *
dmz::WebServicesModuleQt::State::get_pending (const UInt64 RequestId) {

   PendingStruct *ps (pendingTable.lookup (RequestId));

   if (!ps && RequestId) {

      ps = new PendingStruct;
      ps->requestId = RequestId;

      if (!pendingTable.store (RequestId, ps)) { delete ps; ps = 0; }
   }

   return ps;
}


dmz::Boolean
dmz::WebServicesModuleQt::State::is_object_pending (const Handle ObjectHandle) {

   Boolean result (False);

   HashTableUInt64Iterator it;
   PendingStruct *ps = pendingTable.get_first (it);

   while (ps && !result) {

      if (ObjectHandle == ps->objectHandle) { result = True; }
      else { ps = pendingTable.get_next (it); }
   }

   return result;
}


dmz::WebServicesModuleQt::WebServicesModuleQt (const PluginInfo &Info, Config &local) :
      QObject (0),
      Plugin (Info),
      TimeSlice (Info),
      ArchiveObserverUtil (Info, local),
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

   // connect (
   //    &_state.syncTimer, SIGNAL (timeout ()),
   //    this, SLOT (_fetch_changes ()));
   //
   // _state.syncTimer.start (1000);

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

      _fetch_changes (_state.lastSeq);
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

   if (objMod && _state.archiveMod && _state.updateTable.get_count ()) {

      Config global = _state.archiveMod->create_archive (_state.archiveHandle);
      Config objectList;

      if (global.lookup_all_config ("archive.object", objectList)) {

         ConfigIterator it;
         Config object;

         while (objectList.get_next_config (it, object)) {

            UUID uuid = config_to_string ("uuid", object);
            String docId = uuid.to_string ();

            Handle objHandle = objMod->lookup_handle_from_uuid (docId);

            if (_state.updateTable.contains (objHandle)) {

               if (!_state.is_object_pending (objHandle)) {

                  Config archive ("archive");
                  archive.add_config (object);

                  UInt64 requestId = _publish_document (docId, archive);
                  if (requestId) {

                     PendingStruct *ps = _state.get_pending (requestId);
                     if (ps) { ps->objectHandle = objHandle; }
                  }

                  _state.updateTable.remove (objHandle);
               }
            }
         }
      }
   }

   String id;
   if (_state.fetchTable.get_first (id)) {

      _state.fetchTable.remove (id);
      _fetch_document (id);
   }

   _state.fetchChangesDelta += TimeDelta;
   if (_state.fetchChangesDelta > LocalFetchChangesInterval) {

      _state.fetchChangesDelta = 0;
      _fetch_changes (_state.lastSeq);
   }
}


// ArchiveObserver Interface
void
dmz::WebServicesModuleQt::pre_process_archive (
      const Handle ArchiveHandle,
      const Int32 Version) {

}


void
dmz::WebServicesModuleQt::process_archive (
      const Handle ArchiveHandle,
      const Int32 Version,
      Config &local,
      Config &global) {

}


void
dmz::WebServicesModuleQt::post_process_archive (
      const Handle ArchiveHandle,
      const Int32 Version) {

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

      _add_update (ObjectHandle);
   }
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

   if (_state.includeTable.contains (ObjectHandle)) {

      if (Locality == ObjectRemote) {

         _state.updateTable.remove (ObjectHandle);
         _state.includeTable.remove (ObjectHandle);
      }
      else {

      }
   }
}


void
dmz::WebServicesModuleQt::link_objects (
      const Handle LinkHandle,
      const Handle AttributeHandle,
      const UUID &SuperIdentity,
      const Handle SuperHandle,
      const UUID &SubIdentity,
      const Handle SubHandle) {

   _add_update (SuperHandle);
   _add_update (SubHandle);
}


void
dmz::WebServicesModuleQt::unlink_objects (
      const Handle LinkHandle,
      const Handle AttributeHandle,
      const UUID &SuperIdentity,
      const Handle SuperHandle,
      const UUID &SubIdentity,
      const Handle SubHandle) {

   _add_update (SuperHandle);
   _add_update (SubHandle);
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

   _add_update (SuperHandle);
   _add_update (SubHandle);
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

_state.log.error << "---------- _handle_reply[" << RequestId << "][" << StatusCode << "] ----------" << endl;

   PendingStruct *ps = _state.pendingTable.lookup (RequestId);

   String error = config_to_string ("error", Global);
   if (error) {

      String msg = config_to_string ("reason", Global);
_state.log.error << "_handle_reply error: " << msg << endl;
   }
   else if (ps) {

      String docId;
      String docRev;

      if (ps->requestType == "fetchChanges") {

         _handle_changes (RequestId, Global);
      }
      else if (ps->requestType == "fetchDocument") {

         docId = config_to_string ("_id", Global);
         docRev = config_to_string ("_rev", Global);

         String docType = config_to_string ("type", Global);

         Config data;
         Global.lookup_config (docType, data);

         if (docType == "archive") {

            _handle_archive (RequestId, data);
         }
         else {

_state.log.error << "_handle_reply error: doc type unknown: " << docType << endl;
         }
      }
      else {

         docId = config_to_string ("id", Global);
         docRev = config_to_string ("rev", Global);
      }

      if (docId && docRev) {

         _state.revisionMap.insert (docId.get_buffer (), docRev);

_state.log.warn << " docId: " << docId << endl;
_state.log.warn << "docRev: " << docRev << endl;
      }
   }

   ps = _state.pendingTable.remove (RequestId);
   if (ps) { delete ps; ps = 0; }
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
dmz::UInt64
dmz::WebServicesModuleQt::_publish_document (const String &Id, const Config &Data) {

   UInt64 requestId (0);

   if (Id && _state.client) {

      Config doc ("global");

      doc.store_attribute ("_id", Id);

      String rev = _state.revisionMap.value (Id.get_buffer ());
      if (rev) { doc.store_attribute ("_rev", rev); }

      doc.store_attribute ("type", Data.get_name ());

      doc.add_config (Data);

      String json;
      StreamString out (json);

      if (format_config_to_json (doc, out, ConfigStripGlobal, &_state.log)) {

         QUrl url (_state.baseUrl);
         url.setPath (QString ("/db/test/%1").arg (Id.get_buffer ()));

         QByteArray data (json.get_buffer ());

         requestId = _state.client->put (url, data);

         if (requestId) {

            PendingStruct *ps = _state.get_pending (requestId);
            if (ps) {

               ps->docId = Id;
               ps->requestType = "publishDocument";
            }
         }
      }
   }

   return requestId;
}


dmz::UInt64
dmz::WebServicesModuleQt::_fetch_document (const String &Id) {

   UInt64 requestId (0);
   ObjectModule *objMod (get_object_module ());

   if (Id && _state.client && objMod) {

      QUrl url (_state.baseUrl);
      url.setPath (QString ("/db/test/%1").arg (Id.get_buffer ()));

      requestId = _state.client->get (url);

      if (requestId) {

         PendingStruct *ps = _state.get_pending (requestId);
         if (ps) {

            UUID uuid (Id);

            ps->docId = Id;
            ps->objectHandle = objMod->lookup_handle_from_uuid (uuid);
            ps->requestType = "fetchDocument";
         }
      }
   }

   return requestId;
}


dmz::UInt64
dmz::WebServicesModuleQt::_fetch_changes (const Int32 Since) {

   UInt64 requestId (0);

   if (_state.client) {

      QUrl url (_state.baseUrl);
      url.setPath ("/db/test/_changes");
      url.addQueryItem ("since", QString::number (Since));

      requestId = _state.client->get (url);

      if (requestId) {

         PendingStruct *ps = _state.get_pending (requestId);
         if (ps) { ps->requestType = "fetchChanges"; }
      }
   }

   return requestId;
}


void
dmz::WebServicesModuleQt::_handle_archive (
      const UInt64 RequestId,
      const Config &Archive) {

_state.log.error << "---------- _handle_archive[" << RequestId << "] ----------" << endl;
_state.log.warn << "archiveHandle: " << _state.archiveHandle << endl;

   if (RequestId && _state.archiveMod) {

      Config global ("dmz");
      global.add_config (Archive);

       _state.archiveMod->process_archive (_state.archiveHandle, global);
   }
}


void
dmz::WebServicesModuleQt::_handle_changes (const UInt64 RequestId, const Config &Global) {

_state.log.error << "---------- _handle_changes[" << RequestId << "] ----------" << endl;

   _state.lastSeq = config_to_int32 ("last_seq", Global);

   ConfigIterator it;
   Config data;

   while (Global.get_next_config (it, data)) {

      String id = config_to_string ("id", data);
      _state.log.warn << "id: " << id << endl;
      _state.fetchTable.add (id);
   }
}


void
dmz::WebServicesModuleQt::_add_update (const Handle &ObjectHandle) {

   if (_state.includeTable.contains (ObjectHandle)) {

_state.log.error << "_add_update: " << ObjectHandle << endl;
      _state.updateTable.add (ObjectHandle);
   }
}


void
dmz::WebServicesModuleQt::_init (Config &local) {

   setObjectName (get_plugin_name ().get_buffer ());

   Definitions defs (get_plugin_runtime_context ());
   RuntimeContext *context (get_plugin_runtime_context ());

   _state.archiveHandle = defs.create_named_handle (
      config_to_string ("archive.name", local, WebServicesArchiveName));

   // _state.defaultArchiveHandle = defs.create_named_handle (
   //    config_to_string ("archive.name", local, ArchiveDefaultName));

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
