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
#include <dmzRuntimeUUID.h>
#include <dmzTypesHashTableHandleTemplate.h>
#include <dmzTypesHashTableStringTemplate.h>
#include <dmzTypesHashTableUInt64Template.h>
#include <dmzTypesString.h>
#include <dmzTypesUUID.h>
#include <dmzSystem.h>
#include <dmzSystemStreamString.h>
#include <dmzWebServicesConsts.h>
#include "dmzWebServicesModuleQt.h"
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

   static const Float64 LocalFetchChangesInterval (10.0);
};


struct dmz::WebServicesModuleQt::DocStruct {

   String Id;
   String rev;
   Boolean pending;

   DocStruct (const String &DocId) : Id (DocId), pending (False) {;}
};


struct dmz::WebServicesModuleQt::RequestStruct {

   const UInt64 Id;
   const String Type;
   const String DocId;
   String docRev;
   WebServicesObserver &obs;
   Int32 statusCode;
   Boolean error;
   String errorMessage;
   Config data;

   RequestStruct (
      const UInt64 TheRequestId,
      const String &TheRequestType,
      const String &TheDocId,
      WebServicesObserver &theObs) :
      Id (TheRequestId),
      Type (TheRequestType),
      DocId (TheDocId),
      obs (theObs),
      statusCode (0),
      error (False),
      data ("global") {;}
};


struct dmz::WebServicesModuleQt::State {

   const UUID SysId;
   Log log;
   Definitions defs;
   QtHttpClient *client;
   QUrl serverUrl;
   String serverPath;
   String serverDatabase;
   ArchiveModule *archiveMod;
   Handle archiveHandle;
   HashTableStringTemplate<DocStruct> documentTable;
   HashTableUInt64Template<RequestStruct> requestTable;
   StringContainer fetchTable;
   Int32 lastSeq;
   Float64 fetchChangesDelta;
   Boolean loggedIn;
   RequestStruct *continuousFeed;
   RequestStruct *request;

   State (const PluginInfo &Info);
   ~State ();

   DocStruct *get_doc (const String &Id);

   RequestStruct *get_request (
      const UInt64 RequestId,
      const String &ReqeustType,
      const String &DocId,
      WebServicesObserver &obs);
};


dmz::WebServicesModuleQt::State::State (const PluginInfo &Info) :
      SysId (get_runtime_uuid (Info)),
      log (Info),
      defs (Info),
      client (0),
      serverUrl (LocalHost),
      serverPath ("/"),
      serverDatabase (Info.get_name ()),
      archiveMod (0),
      archiveHandle (0),
      fetchChangesDelta (0),
      lastSeq (0) ,
      loggedIn (False),
      continuousFeed (0),
      request (0) {

}


dmz::WebServicesModuleQt::State::~State () {

   request = 0;
   continuousFeed = 0;

   requestTable.empty ();
   documentTable.empty ();

   if (client) { client->deleteLater (); client = 0; }
}


dmz::WebServicesModuleQt::DocStruct *
dmz::WebServicesModuleQt::State::get_doc (const String &Id) {

   DocStruct *ds (documentTable.lookup (Id));
   if (!ds && Id) {

      ds = new DocStruct (Id);
      if (!documentTable.store (ds->Id, ds)) { delete ds; ds = 0; }
   }

   return ds;
}


dmz::WebServicesModuleQt::RequestStruct *
dmz::WebServicesModuleQt::State::get_request (
      const UInt64 RequestId,
      const String &RequestType,
      const String &DocId,
      WebServicesObserver &obs) {

   RequestStruct *request (requestTable.lookup (RequestId));
   if (!request && RequestId) {

      request = new RequestStruct (RequestId, RequestType, DocId, obs);
      if (!requestTable.store (RequestId, request)) { delete request; request = 0; }

      DocStruct *doc = get_doc (DocId);
      if (doc) { doc->pending = True; }
   }

   return request;
}


dmz::WebServicesModuleQt::WebServicesModuleQt (const PluginInfo &Info, Config &local) :
      QObject (0),
      Plugin (Info),
      TimeSlice (Info),
      WebServicesModule (Info),
      WebServicesObserver (Info),
      _state (*(new State (Info))) {

   _state.client = new QtHttpClient (Info, this);

   connect (
      _state.client,
      SIGNAL (reply_download_progress (const UInt64, QNetworkReply *, qint64, qint64)),
      this,
      SLOT (_reply_download_progress (const UInt64, QNetworkReply *, qint64, qint64)));

   connect (
      _state.client, SIGNAL (reply_finished (const UInt64, QNetworkReply *)),
      this, SLOT (_reply_finished (const UInt64, QNetworkReply *)));

   connect (
      _state.client, SIGNAL (reply_aborted (const UInt64)),
      this, SLOT (_reply_aborted (const UInt64)));

   connect (
      _state.client, SIGNAL (authentication_required (QNetworkReply *, QAuthenticator *)),
      this, SLOT (_authenticate (QNetworkReply *, QAuthenticator *)));

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

      if (_state.client) {

         String DocId ("_session");
         QUrl url (_get_root_url (DocId));

         QMap<QString, QString> params;
         params.insert ("name", "bordergame");
         params.insert ("password", "couch4me");

         QNetworkRequest request = _state.client->get_next_request (url);

         request.setHeader (
            QNetworkRequest::ContentTypeHeader,
            "application/x-www-form-urlencoded");

         UInt64 requestId = _state.client->post (request, params);
         if (requestId) {

            _state.get_request (requestId, "postSession", DocId, *this);
         }
      }
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

//                     RequestStruct *rs = _state.get_request (requestId);
//                     if (rs) { rs->objectHandle = objHandle; }
                  }

                  _state.updateTable.remove (objHandle);
               }
            }
         }
      }
   }

#endif

//   _state.fetchChangesDelta += TimeDelta;
//   if (_state.fetchChangesDelta > LocalFetchChangesInterval) {

//      _state.fetchChangesDelta = 0;

//      if (!_state.continuousFeed && _state.loggedIn) {

//         _state.continuousFeed = _fetch_changes (_state.lastSeq, True);
//      }
//   }
}


// WebServicesModule Interface
dmz::Boolean
dmz::WebServicesModuleQt::publish_config (
      const String &Id,
      const Config &Data,
      WebServicesObserver &obs) {

   _state.log.warn << "publish: " << Id << endl;
   return _publish_document (Id, Data, obs) ? True : False;
}


dmz::Boolean
dmz::WebServicesModuleQt::fetch_config (const String &Id, WebServicesObserver &obs) {

   _state.log.warn << "fetch: " << Id << endl;
   return _fetch_document (Id, obs) ? True : False;
}


dmz::Boolean
dmz::WebServicesModuleQt::fetch_configs (
      const StringContainer &IdList,
      WebServicesObserver &obs) {

   Boolean error (False);

   String id;
   StringContainerIterator it;

   while (IdList.get_next (it, id)) {

      if (!_fetch_document (id, obs)) { error = True; }
   }

   return !error;
}


dmz::Boolean
dmz::WebServicesModuleQt::delete_config (const String &Id, WebServicesObserver &obs) {

   _state.log.warn << "delete: " << Id << endl;
   return _delete_document (Id, obs) ? True : False;
}


dmz::Boolean
dmz::WebServicesModuleQt::delete_configs (
      const StringContainer &IdList,
      WebServicesObserver &obs) {

   Boolean error (False);

   String id;
   StringContainerIterator it;

   while (IdList.get_next (it, id)) {

      if (!_delete_document (id, obs)) { error = True; }
   }

   return !error;
}


void
dmz::WebServicesModuleQt::_authenticate (
      QNetworkReply *reply,
      QAuthenticator *authenticator) {

//   String name ("bordergame");
//   String password ("couch4me");

//   authenticator->setUser (name.get_buffer ());
//   authenticator->setPassword (password.get_buffer ());

   _state.log.error << "Authentication Required!!!" << endl;
}

void
dmz::WebServicesModuleQt::_reply_download_progress (
      const UInt64 RequestId,
      QNetworkReply *reply,
      qint64 bytesReceived,
      qint64 bytesTotal) {

   if (_state.continuousFeed && reply) {

      if (_state.continuousFeed->Id == RequestId) {

         Boolean done = False;
         QByteArray data = reply->readLine (bytesReceived);
         while (!data.isEmpty () && !done) {

            String jsonData (data.constData ());

            Config global ("global");
            if (json_string_to_config (jsonData, global)) {

               _state.continuousFeed->data = global;
               done = _handle_continuous_feed (*(_state.continuousFeed));
            }

            if (!done) { data = reply->readLine (); }
         }

         if (done) {

            reply->readAll ();

            _state.requestTable.remove (_state.continuousFeed->Id);
            delete _state.continuousFeed;
            _state.continuousFeed = 0;
         }
      }
   }
}


void
dmz::WebServicesModuleQt::_reply_aborted (const UInt64 RequestId) {

   _state.log.error << "_reply_aborted: " << RequestId << endl;
}


void
dmz::WebServicesModuleQt::_reply_finished (
      const UInt64 RequestId,
      QNetworkReply *reply) {

   RequestStruct *request (_state.requestTable.lookup (RequestId));
   if (reply && request) {

      request->statusCode =
         reply->attribute (QNetworkRequest::HttpStatusCodeAttribute).toInt();

      if (request->statusCode > 300) {

_state.log.warn << "StatusCode: " << request->statusCode << endl;
      }

      QByteArray data (reply->readAll ());
      const String JsonData (data.constData ());

      if (JsonData) {

//         request->data = Config ("global");
         if (json_string_to_config (JsonData, request->data)) {

            _handle_reply (*request);
         }
         else {

            request->data = Config ("global");
            request->data.store_attribute ("error", "Parse Error");
            request->data.store_attribute ("reason", "Error parsing json data.");

            _handle_reply (*request);
         }
      }
      else {

         String reason = qPrintable (reply->errorString ());

         request->data = Config ("global");
         request->data.store_attribute ("error", "Network Error");
         request->data.store_attribute ("reason", reason);

         _handle_reply (*request);
      }

      if (request->docRev) {

         DocStruct *doc = _state.get_doc (request->DocId);
         if (doc) {

            doc->rev = request->docRev;
            doc->pending = False;
         }
      }

      if (_state.request == request) { _state.request = 0; }
   }

   request = _state.requestTable.remove (RequestId);
   if (request) { delete request; request = 0; }
}


void
dmz::WebServicesModuleQt::_handle_reply (RequestStruct &request) {

   String msg;
   request.error = request.data.lookup_attribute ("error", msg);

   if (request.error) {

      _state.log.error << "Error: " << msg << endl;
//      _state.log.info << request.data << endl;
   }

   if (request.Type == "postSession") {

      if (config_to_boolean ("ok", request.data)) {

//         String name = config_to_string ("name", request.data);

//         _state.loggedIn = True;
//         _state.loginSuccessfullMsg.send ();
      }
      else {

//         _state.loginFailedMsg.send ();
      }
   }
   else if (request.Type == "fetchChanges") {

      _changes_fetched (request);
   }
   else if (request.Type == "fetchDocument") {

       _document_fetched (request);
   }
   else if (request.Type == "deleteDocument") {

      _document_deleted (request);
   }
   else if (request.Type == "publishDocument") {

      _document_published (request);
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
dmz::WebServicesModuleQt::RequestStruct *
dmz::WebServicesModuleQt::_publish_document (
      const String &Id,
      const Config &Data,
      WebServicesObserver &obs) {

   RequestStruct *request (0);

   if (Id && _state.client) {

      Config doc ("global");

      doc.store_attribute ("_id", Id);

      DocStruct *ds = _state.get_doc (Id);
      if (ds && ds->rev) { doc.store_attribute ("_rev", ds->rev); }

      doc.store_attribute ("system_id", _state.SysId.to_string ());

      doc.store_attribute ("type", Data.get_name ());

      doc.add_config (Data);

      String json;
      StreamString out (json);

      if (format_config_to_json (doc, out, ConfigStripGlobal, &_state.log)) {

         QUrl url = _get_url (Id);
         QByteArray data (json.get_buffer ());

         const UInt64 RequestId (_state.client->put (url, data));
         if (RequestId) {

            request = _state.get_request (RequestId, "publishDocument", Id, obs);
         }
      }
   }

   return request;
}


dmz::WebServicesModuleQt::RequestStruct *
dmz::WebServicesModuleQt::_fetch_document (const String &Id, WebServicesObserver &obs) {

   RequestStruct *request;

   if (Id && _state.client) {

      QUrl url = _get_url (Id);

      UInt64 requestId = _state.client->get (url);
      if (requestId) {

         request = _state.get_request (requestId, "fetchDocument", Id, obs);
      }
   }

   return request;
}


dmz::WebServicesModuleQt::RequestStruct *
dmz::WebServicesModuleQt::_delete_document (const String &Id, WebServicesObserver &obs) {

   RequestStruct *request;

   if (Id && _state.client) {

      DocStruct *doc = _state.get_doc (Id);
      if (doc && doc->rev) {

         QUrl url = _get_url (Id);
         url.addQueryItem ("rev", doc->rev.get_buffer ());

         UInt64 requestId = _state.client->del (url);
         if (requestId) {

            request = _state.get_request (requestId, "deleteDocument", Id, obs);
         }
      }
   }

   return request;
}


dmz::WebServicesModuleQt::RequestStruct *
dmz::WebServicesModuleQt::_fetch_changes (const Int32 Since, const Boolean Continuous) {

   RequestStruct *request;

   if (_state.client) {

      QUrl url = _get_url ("_changes");
      if (Continuous) { url.addQueryItem ("feed", "continuous"); }
      url.addQueryItem ("since", QString::number (Since));

      UInt64 requestId = _state.client->get (url);
      if (requestId) {

         String requestType = "fetchChanges";
         if (Continuous) { requestType << "Continuous"; }

         request = _state.get_request (requestId, "fetchChanges", "_changes", *this);
      }
   }

   return request;
}


void
dmz::WebServicesModuleQt::_document_published (RequestStruct &request) {

   if (!request.error) {

      String docId = config_to_string ("id", request.data);

      if (request.DocId == docId) {

         request.docRev = config_to_string ("rev", request.data);
      }
   }

   request.obs.config_published (request.DocId, request.error, request.data);
}


void
dmz::WebServicesModuleQt::_document_fetched (RequestStruct &request) {

   if (request.error) {

      request.obs.config_fetched (request.DocId, True, request.data);
   }
   else {

      String docId = config_to_string ("_id", request.data);

      if (request.DocId == docId) {

         request.docRev = config_to_string ("_rev", request.data);
      }

      String systemId = config_to_string ("system_id", request.data);

      String docType = config_to_string ("type", request.data);

      Config data;
      request.data.lookup_config (docType, data);

      request.obs.config_fetched (request.DocId, False, data);
   }
}


void
dmz::WebServicesModuleQt::_document_deleted (RequestStruct &request) {

//   if (config_to_boolean ("ok", request.data)) {

//   }

   if (!request.error) {

      String docId = config_to_string ("id", request.data);

      if (request.DocId == docId) {

         request.docRev = config_to_string ("rev", request.data);
      }
   }

   request.obs.config_deleted (request.DocId, request.error, request.data);
}


//void
//dmz::WebServicesModuleQt::_archive_fetched (
//      const UInt64 RequestId,
//      const Config &Archive) {

//   if (RequestId && _state.archiveMod) {

//      Config global ("dmz");
//      global.add_config (Archive);

//      _state.archiveMod->process_archive (_state.archiveHandle, global);
//   }
//}


//void
//dmz::WebServicesModuleQt::_session_fetched (
//      const UInt64 RequestId,
//      const Config &Session) {

//_state.log.error << "---------- _handle_session[" << RequestId << "] ----------" << endl;

//   if (RequestId) {

//      const String DocId (config_to_string ("uuid", Session));
//      const String Name  (config_to_string ("name", Session, "Unknown Action"));

//      Config actionList;
//      if (Session.lookup_all_config ("action", actionList)) {

//         RuntimeContext *context (get_plugin_runtime_context ());

//         ConfigIterator it;
//         Config action;

//         while (Session.get_next_config (it, action)) {

//            const Message Type (
//               config_create_message ("message", action, "", context, &_state.log));

//            const Handle Target (config_to_named_handle ("target", action, "", context));

//            Data value;
//            const Boolean DataCreated (
//               config_to_data ("data", action, context, value, &_state.log));

//            if (Type) {

//_state.log.warn << "sent message: " << Type.get_name () << " to: " << Target << endl;
//               Type.send (Target, DataCreated ? &value : 0, 0);
//            }
//         }
//      }
//   }
//}


void
dmz::WebServicesModuleQt::_changes_fetched (RequestStruct &request) {

   if (request.error) {

   }
   else {

      _state.lastSeq = config_to_int32 ("last_seq", request.data, _state.lastSeq);

      ConfigIterator it;
      Config data;

      while (request.data.get_next_config (it, data)) {

         const String Id  (config_to_string ("id", data));
         const String Rev (config_to_string ("changes.rev", data));
         const Boolean Deleted (config_to_boolean ("deleted", data));

         DocStruct *doc = _state.get_doc (Id);
         if (doc) {

            if (!Deleted && !doc->pending && (doc->rev != Rev)) {

   _state.log.warn << "changes for id: " << Id << endl;
//               _state.fetchTable.add (id);
            }
         }
      }
   }
}


dmz::Boolean
dmz::WebServicesModuleQt::_handle_continuous_feed (RequestStruct &request) {

   Boolean done (False);

   const Config Feed (request.data);

   Int32 lastSeq = config_to_int32 ("last_seq", Feed, -1);
   if (lastSeq == -1) {

      _state.lastSeq = config_to_int32 ("seq", Feed, _state.lastSeq);

      String id = config_to_string ("id", Feed);
      String rev = config_to_string ("changes.rev", Feed);
      Boolean deleted = config_to_boolean ("deleted", Feed);

      DocStruct *doc = _state.get_doc (id);
      if (doc) {

         if (!deleted && !doc->pending && (doc->rev != rev)) {

            _state.fetchTable.add (id);
         }
      }
   }
   else {

      _state.lastSeq = lastSeq;
      done = True;
   }

   return done;
}


QUrl
dmz::WebServicesModuleQt::_get_url (const String &EndPoint) const {

   QUrl url (_state.serverUrl);

   String path;
   path << _state.serverPath << _state.serverDatabase  << "/" << EndPoint;

   url.setPath (path.get_buffer ());
   return url;
}


QUrl
dmz::WebServicesModuleQt::_get_root_url (const String &EndPoint) const {

   QUrl url (_state.serverUrl);

   String path;
   path << _state.serverPath << EndPoint;

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
