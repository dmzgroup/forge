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
   HashTableUInt64Template<RequestStruct> feedTable;
   StringContainer fetchTable;
   Int32 lastSeq;
   Float64 fetchChangesDelta;
   Boolean loggedIn;
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
      request (0) {

}


dmz::WebServicesModuleQt::State::~State () {

   request = 0;

   feedTable.clear ();
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

   return _publish_document (Id, Data, obs) ? True : False;
}


dmz::Boolean
dmz::WebServicesModuleQt::fetch_config (const String &Id, WebServicesObserver &obs) {

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


dmz::Boolean
dmz::WebServicesModuleQt::get_config_updates (
      WebServicesObserver &obs,
      const Int32 Since,
      const Boolean Heavy) {

   return _fetch_changes (obs, Since, Heavy) ? True : False;
}


dmz::Boolean
dmz::WebServicesModuleQt::start_realtime_updates (
      WebServicesObserver &obs,
      const Int32 Since) {

   Boolean result (False);

   if (_state.client) {

      const String Id ("_changes");
      QUrl url = _get_url (Id);
      url.addQueryItem ("since", QString::number (Since));
      url.addQueryItem ("feed", "continuous");
      url.addQueryItem ("heartbeat", "50000");

      UInt64 requestId = _state.client->get (url);
      if (requestId) {

         String requestType = "fetchChangesContinuous";

         RequestStruct *request = _state.get_request (requestId, requestType, Id, obs);
         if (request) {

            _state.feedTable.store (request->Id, request);
         }
      }
   }

   return result;
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

   RequestStruct *feed = _state.feedTable.lookup (RequestId);
   if (feed && reply) {

      Boolean done = False;
      QByteArray data = reply->readLine (bytesReceived);
      while (!data.isEmpty () && !done) {

         String jsonData (data.constData ());

_state.log.info << feed->DocId << " : " << feed->Id << endl;
_state.log.info << "json: " <<jsonData << endl;
         Config global ("global");
         if (json_string_to_config (jsonData, global)) {

            feed->data = global;
            done = _handle_continuous_feed (*feed);
         }

         if (!done) { data = reply->readLine (); }
      }

      if (done) {

         reply->readAll ();

         _state.feedTable.remove (feed->Id);
         _state.requestTable.remove (feed->Id);
         delete feed; feed = 0;
      }
   }
   else {

      _state.log.info << bytesReceived << "/" << bytesTotal << endl;
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

//      if (request->statusCode > 300) {

_state.log.warn << "StatusCode: " << request->statusCode << endl;
_state.log.info << request->DocId << " : " << request->Id << endl;
//      }

      QByteArray data (reply->readAll ());
      const String JsonData (data.constData ());

      if (JsonData) {

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
_state.log.error << reason << endl;
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

   if (request.Type == "fetchDocument") {

       _document_fetched (request);
   }
   else if (request.Type == "publishDocument") {

      _document_published (request);
   }
   else if (request.Type == "deleteDocument") {

      _document_deleted (request);
   }
   else if (request.Type == "fetchChanges") {

      _changes_fetched (request);
   }
   else if (request.Type == "fetchChangesHeavy") {

      _changes_fetched_heavy (request);
   }
   else if (request.Type == "postSession") {

      if (config_to_boolean ("ok", request.data)) {

//         String name = config_to_string ("name", request.data);

//         _state.loggedIn = True;
//         _state.loginSuccessfullMsg.send ();
      }
      else {

//         _state.loginFailedMsg.send ();
      }
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

      doc.store_attribute ("runtime_id", _state.SysId.to_string ());

      doc.store_attribute ("type", Data.get_name ());

      doc.add_config (Data);

      String json;
      StreamString out (json);

      if (format_config_to_json (doc, out, ConfigStripGlobal, &_state.log)) {

         QUrl url = _get_url (Id);
         QNetworkRequest req = _state.client->get_next_request (url);

         DocStruct *ds = _state.get_doc (Id);
         if (ds && ds->rev) { req.setRawHeader ("If-Match", ds->rev.get_buffer ()); }

         QByteArray data (json.get_buffer ());

         const UInt64 RequestId (_state.client->put (req, data));
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
         QNetworkRequest req = _state.client->get_next_request (url);

         req.setRawHeader ("If-Match", doc->rev.get_buffer ());

         UInt64 requestId = _state.client->del (req);
         if (requestId) {

            request = _state.get_request (requestId, "deleteDocument", Id, obs);
         }
      }
   }

   return request;
}


dmz::WebServicesModuleQt::RequestStruct *
dmz::WebServicesModuleQt::_fetch_changes (
      WebServicesObserver &obs,
      const Int32 Since,
      const Boolean Heavy) {

   RequestStruct *request;

   if (_state.client) {

      const String Id ("_changes");
      QUrl url = _get_url (Id);
      url.addQueryItem ("since", QString::number (Since));

      if (Heavy) { url.addQueryItem ("include_docs", "true"); }

      UInt64 requestId = _state.client->get (url);
      if (requestId) {

         String requestType = "fetchChanges";
         if (Heavy) { requestType << "Heavy"; }

         request = _state.get_request (requestId, requestType, Id, obs);
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


void
dmz::WebServicesModuleQt::_changes_fetched (RequestStruct &request) {

   if (request.error) {

   }
   else {

      StringContainer updatedList;
      StringContainer deletedList;

      ConfigIterator it;
      Config data;

      while (request.data.get_next_config (it, data)) {

         const String Id  (config_to_string ("id", data));
         const String Rev (config_to_string ("changes.rev", data));
         const Boolean Deleted (config_to_boolean ("deleted", data));

         if (Deleted) {

            DocStruct *doc = _state.documentTable.remove (Id);
            if (doc) { delete doc; doc = 0; }
            deletedList.add (Id);
         }
         else {

            DocStruct *doc = _state.get_doc (Id);
            if (doc) {

               if (!doc->pending && (doc->rev != Rev)) { updatedList.add (Id); }
            }
         }
      }

      UInt32 lastSeq = config_to_int32 ("last_seq", request.data);
      request.obs.config_updated (updatedList, deletedList, lastSeq);
//      request.obs.last_config_update (lastSeq);
   }
}


void
dmz::WebServicesModuleQt::_changes_fetched_heavy (RequestStruct &request) {

   _state.log.error << "convert json string to config" << endl;

qApp->processEvents ();

   if (request.error) {

   }
   else {

      ConfigIterator it;
      Config resultDoc;

      while (request.data.get_next_config (it, resultDoc)) {

         const String DocId (config_to_string ("id", resultDoc));
         const UInt32 Seq (config_to_int32 ("seq", resultDoc));
         const Boolean Deleted (config_to_boolean ("deleted", resultDoc));

         Config docData;
         resultDoc.lookup_config ("doc", docData);

         if (Deleted) {

            DocStruct *doc = _state.documentTable.remove (DocId);
            if (doc) { delete doc; doc = 0; }

            request.obs.config_updated (DocId, True, Seq, docData);
         }
         else {

            DocStruct *doc = _state.get_doc (DocId);
            if (doc) {

               doc->rev = config_to_string ("_rev", docData);
            }

            String runtimeId = config_to_string ("runtime_id", docData);

            String docType = config_to_string ("type", docData);

            Config data;
            docData.lookup_config (docType, data);

            request.obs.config_updated (DocId, False, Seq, data);
         }
      }

      UInt32 lastSeq = config_to_int32 ("last_seq", request.data);
//      request.obs.last_config_update (lastSeq);
   }
}


dmz::Boolean
dmz::WebServicesModuleQt::_handle_continuous_feed (RequestStruct &request) {

   Boolean done (True);

   const Config Feed (request.data);

   Int32 lastSeq = config_to_int32 ("last_seq", Feed, -1);
   if (lastSeq == -1) {

      done = False;
      lastSeq = config_to_int32 ("seq", Feed);

      const String Id (config_to_string ("id", Feed));
      const String Rev (config_to_string ("changes.rev", Feed));
      const Boolean Deleted (config_to_boolean ("deleted", Feed));

      Config data;

      if (Deleted) {

         DocStruct *doc = _state.documentTable.remove (Id);
         if (doc) {

            delete doc; doc = 0;
            request.obs.config_updated (Id, True, lastSeq, data);
         }
      }
      else {

         DocStruct *doc = _state.get_doc (Id);
         if (doc) {

            if (!doc->pending && (doc->rev != Rev)) {

               doc->rev = Rev;
               request.obs.config_updated (Id, False, lastSeq, data);
            }
         }
      }
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
