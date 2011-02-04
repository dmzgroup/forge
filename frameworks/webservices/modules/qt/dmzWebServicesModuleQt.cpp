#include <dmzArchiveModule.h>
#include <dmzFoundationJSONUtil.h>
#include <dmzObjectAttributeMasks.h>
#include <dmzObjectModule.h>
#include "dmzQtHttpClient.h"
#include <dmzRuntimeConfig.h>
#include <dmzRuntimeConfigToNamedHandle.h>
#include <dmzRuntimeConfigToTypesBase.h>
#include <dmzRuntimeConfigWrite.h>
#include <dmzRuntimeData.h>
#include <dmzRuntimeDefinitions.h>
#include <dmzRuntimeHandle.h>
#include <dmzRuntimeMessaging.h>
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
#include <dmzSystem.h>
#include <dmzSystemStreamString.h>
#include <dmzWebServicesConsts.h>
#include "dmzWebServicesModuleQt.h"
#include <dmzWebServicesObserver.h>
#include <QtCore/QtCore>
#include <QtNetwork/QtNetwork>

using namespace dmz;


namespace {

   static const char LocalHost[]  ="http://localhost:5984";

   static const String Local_Id ("_id");
   static const String Local_Rev ("_rev");
   static const String Local_Attachments ("_attachments");

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

   struct RecordStruct {

      const Message Type;
      const Handle Target;
      Data *data;

      RecordStruct (
            const Message &TheType,
            const Handle TheHandle,
            const Data *TheData) :
            Type (TheType),
            Target (TheHandle),
            data (TheData ? new Data (*TheData) : 0) {;}

      ~RecordStruct () { if (data) { delete data; data = 0; } }
   };

   struct SessionStruct {

      const String Name;
      const RuntimeHandle SessionHandle;
      QList<RecordStruct *> recordList;
      String docId;
      UInt64 requestId;


      SessionStruct (const String &TheName, RuntimeContext *context) :
            Name (TheName),
            SessionHandle (TheName + ".SessionHandle", context),
            requestId (0) {

         UUID uuid;
         create_uuid (uuid);
         docId = uuid.to_string ();
      }

      ~SessionStruct () {

         qDeleteAll (recordList);
         recordList.clear ();
      }
   };
};


struct dmz::WebServicesModuleQt::State {

   const RuntimeHandle NestedHandle;
   Log log;
   Definitions defs;
   QtHttpClient *client;
   QUrl serverUrl;
   String serverPath;
   String serverDatabase;
   ArchiveModule *archiveMod;
   Handle archiveHandle;
   SessionStruct *session;
   HashTableHandleTemplate<SessionStruct> sessionTable;
   HashTableHandleTemplate<WebServicesObserver> obsTable;
   QMap<QString, String> revisionMap;
   HashTableUInt64Template<PendingStruct> pendingTable;
   StringContainer fetchTable;
   Int32 lastSeq;
   Float64 fetchChangesDelta;

   State (const PluginInfo &Info);
   ~State ();

   PendingStruct *get_pending (const UInt64 RequestId);
   Boolean is_object_pending (const Handle ObjectHandle);

   void update_observers ();
};


dmz::WebServicesModuleQt::State::State (const PluginInfo &Info) :
      NestedHandle ("dmz.WebServices.NestedHandle", Info.get_context ()),
      log (Info),
      defs (Info),
      client (0),
      serverUrl (LocalHost),
      serverPath ("/"),
      serverDatabase (Info.get_name ()),
      archiveMod (0),
      archiveHandle (0),
      session (0),
      fetchChangesDelta (0),
      lastSeq (0) {

}


dmz::WebServicesModuleQt::State::~State () {

   pendingTable.empty ();

   if (session) { delete session; session = 0; }
   if (client) { client->deleteLater (); client = 0; }
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


void
dmz::WebServicesModuleQt::State::update_observers () {

   if (obsTable.get_count ()) {

      HashTableHandleIterator it;
      WebServicesObserver *obs (obsTable.get_first (it));
      while (obs) {

         if (session) { obs->start_session (); }
         else { obs->stop_session (); }

         obs = obsTable.get_next (it);
      }
   }
}


dmz::WebServicesModuleQt::WebServicesModuleQt (const PluginInfo &Info, Config &local) :
      QObject (0),
      Plugin (Info),
      TimeSlice (Info),
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

#if 0
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

#endif

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


// WebServicesModule Interface
dmz::Boolean
dmz::WebServicesModuleQt::register_webservices_observer (WebServicesObserver &observer) {

   Boolean result (False);

   result = _state.obsTable.store (
      observer.get_webservices_observer_handle (),
      &observer);

   return result;
}


dmz::Boolean
dmz::WebServicesModuleQt::release_webservices_observer (WebServicesObserver &observer) {

   Boolean result (False);
   result = _state.obsTable.remove (observer.get_webservices_observer_handle ());
   return result;
}


dmz::Boolean
dmz::WebServicesModuleQt::is_recording () const {

   return _state.session;
}


dmz::Handle
dmz::WebServicesModuleQt::start_session (const String &Name) {

   Handle result (0);

   if (!_state.session) {

      _state.session = new SessionStruct (Name, get_plugin_runtime_context ());
      if (_state.session) {

         _state.update_observers ();

         result = _state.session->SessionHandle.get_runtime_handle ();
      }
   }
   else {

      result = _state.NestedHandle.get_runtime_handle ();
   }

   return result;
}


dmz::Boolean
dmz::WebServicesModuleQt::store_record (
      const Message &Type,
      const Handle Target,
      const Data *Record) {

   Boolean result (False);

   if (_state.session) {

      RecordStruct *record (new RecordStruct (Type, Target, Record));
      if (record) {

_state.log.warn << "------------------: " << "store_rocord: " << Type.get_name () << endl;
         _state.session->recordList.append (record);
         result = True;
      }
   }
}


dmz::Boolean
dmz::WebServicesModuleQt::stop_session (const Handle SessionHandle) {

   Boolean result (False);

   if (_state.session) {

      if (SessionHandle == _state.session->SessionHandle.get_runtime_handle ()) {

         _state.sessionTable.store (SessionHandle, _state.session);
         _state.session = 0;

         _state.update_observers ();

         _publish_session (SessionHandle);

         result = True;
      }
      else if (SessionHandle == _state.NestedHandle.get_runtime_handle ()) {

         result = True;
      }
   }
}


dmz::Boolean
dmz::WebServicesModuleQt::abort_session (const Handle SessionHandle) {

   Boolean result (False);

   if (_state.session) {

      if (SessionHandle == _state.session->SessionHandle.get_runtime_handle ()) {

         SessionStruct *session (_state.session);
         _state.session = 0;

         _state.update_observers ();

         delete session; session = 0;

         result = True;
      }
   }

   return result;
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
         else if (docType == "session") {

            _handle_session (RequestId, data);
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
dmz::WebServicesModuleQt::_publish_session (const Handle SessionHandle) {

   UInt64 requestId (0);

   SessionStruct *session (_state.sessionTable.lookup (SessionHandle));
   if (session) {

      Config sessionDoc ("session");

      sessionDoc.store_attribute ("name", session->Name);
      sessionDoc.store_attribute ("uuid", session->docId);

      foreach (RecordStruct *record, session->recordList) {

         Config action ("action");

         action.store_attribute ("message", record->Type.get_name ());

         if (record->Target) {

            action.store_attribute (
               "target",
               _state.defs.lookup_named_handle_name (record->Target));
         }

         if (record->data) {

            action.add_config (
               data_to_config (
                  *(record->data),
                  get_plugin_runtime_context (),
                  &_state.log));
         }

         sessionDoc.add_config (action);
      }

      requestId = _publish_document (session->docId, sessionDoc);

      session->requestId = requestId;
   }

   return requestId;
}


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

         QUrl url = _get_url (Id);
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

   if (Id && _state.client) {

      QUrl url = _get_url (Id);
      requestId = _state.client->get (url);

      if (requestId) {

         PendingStruct *ps = _state.get_pending (requestId);
         if (ps) {

            UUID uuid (Id);

            ps->docId = Id;
            // ps->objectHandle = objMod->lookup_handle_from_uuid (uuid);
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

      QUrl url = _get_url ("_changes");
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

   _state.log.warn << "_handle_archive" << endl;
   if (RequestId && _state.archiveMod) {

      Config global ("dmz");
      global.add_config (Archive);

      _state.archiveMod->process_archive (_state.archiveHandle, global);
   }
}


void
dmz::WebServicesModuleQt::_handle_session (
      const UInt64 RequestId,
      const Config &Session) {

_state.log.error << "---------- _handle_session[" << RequestId << "] ----------" << endl;

   if (RequestId) {

_state.log.warn << Session << endl;
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


QUrl
dmz::WebServicesModuleQt::_get_url (const String &EndPoint) const {

   QUrl url (_state.serverUrl);

   String path;
   path << _state.serverPath << _state.serverDatabase  << "/" << EndPoint;

   url.setPath (path.get_buffer ());
   return url;
}


void
dmz::WebServicesModuleQt::_init (Config &local) {

   setObjectName (get_plugin_name ().get_buffer ());

   Definitions defs (get_plugin_runtime_context ());
   RuntimeContext *context (get_plugin_runtime_context ());

   Config server;
   if (local.lookup_config ("server", server)) {

      Config proxy;
      if (server.lookup_config ("proxy", proxy)) {

         String host = config_to_string ("host", proxy, "localhost");
         Int32 port = config_to_int32 ("port", proxy, 8888);

         QNetworkProxy proxy;
         proxy.setType (QNetworkProxy::HttpProxy);
         proxy.setHostName (host.get_buffer ());
         proxy.setPort (port);

         QNetworkProxy::setApplicationProxy(proxy);

         _state.log.info << "Using proxy: " << host << ":" << port << endl;
      }

      String host = qPrintable (_state.serverUrl.host ());
      host =  config_to_string ("host", server, host);
      _state.serverUrl.setHost (host.get_buffer ());

      Int32 port = _state.serverUrl.port ();
      port = config_to_int32 ("port", server, port);
      _state.serverUrl.setPort (port);

      if (port == 443) { _state.serverUrl.setScheme ("https"); }

      _state.serverPath = config_to_string ("path", server, _state.serverPath);

      _state.serverDatabase = config_to_string ("db", server, _state.serverDatabase);
   }

   _state.log.info << "Using server: " << qPrintable (_get_url ("").toString ()) << endl;
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
