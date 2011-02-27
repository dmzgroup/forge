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

#define DMZ_WEBSERVICES_DEBUG

#ifdef DMZ_WEBSERVICES_DEBUG
#include <qdb.h>
static dmz::qdb out;
#endif

using namespace dmz;


namespace {

   static const char LocalHost[]  ="http://localhost:5984";

   static const String Local_Id ("_id");
   static const String Local_Rev ("_rev");
   static const String Local_Attachments ("_attachments");

//   static const QString LocalTimeStampFormat ("yyyy-M-d-h-m-s");

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

   const String Id;
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
   QDateTime timeStamp;
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
      timeStamp (QDateTime::currentDateTime ()),
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
   String dateFormat;
   QAuthenticator auth;
   Handle usernameHandle;
   Handle passwordHandle;
   Handle targetHandle;
   Message loginRequiredMsg;
   Message loginMsg;
   Message loginSuccessMsg;
   Message loginFailedMsg;
   Message logoutMsg;
   Boolean loggedIn;
   HashTableStringTemplate<DocStruct> documentTable;
   HashTableUInt64Template<RequestStruct> requestTable;
   HashTableUInt64Template<RequestStruct> feedTable;
   StringContainer fetchTable;
   Int32 lastSeq;
   Float64 fetchChangesDelta;
   RequestStruct *request;

   State (const PluginInfo &Info);
   ~State ();

   DocStruct *get_doc (const String &Id);
   DocStruct *update_doc (const String &Id, const String &Rev);

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
      dateFormat ("ddd, dd MMM yyyy HH:mm:ss 'G''M''T'"),
      usernameHandle (0),
      passwordHandle (0),
      targetHandle (0),
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


dmz::WebServicesModuleQt::DocStruct *
dmz::WebServicesModuleQt::State::update_doc (const String &Id, const String &Rev) {

   DocStruct *ds (get_doc (Id));
   if (ds) { ds->rev = Rev; }
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
      MessageObserver (Info),
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

//      if (!_state.archiveMod) { _state.archiveMod = ArchiveModule::cast (PluginPtr); }
   }
   else if (Mode == PluginDiscoverRemove) {

//      if (_state.archiveMod && (_state.archiveMod == ArchiveModule::cast (PluginPtr))) {

//         _state.archiveMod = 0;
//      }
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


// Message Observer Interface
void
dmz::WebServicesModuleQt::receive_message (
      const Message &Msg,
      const UInt32 MessageSendHandle,
      const Handle TargetObserverHandle,
      const Data *InData,
      Data *outData) {

   if (Msg == _state.loginMsg) {

      if (InData) {

         String username;
         InData->lookup_string (_state.usernameHandle, 0, username);

         String password;
         InData->lookup_string (_state.passwordHandle, 0, password);

         if (username && password) {

            _state.auth.setUser (username.get_buffer ());
            _state.auth.setPassword (password.get_buffer ());

            _fetch_session ();
         }
         else {

            _state.log.warn << "A username and password are both needed to login."
                            << endl;

            _state.loginFailedMsg.send ();
         }
      }
   }
   else if (Msg == _state.logoutMsg) {

      _delete_session ();
   }
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

   Boolean result (False);

   if (IdList.get_count () == 1) {

      String id;
      if (IdList.get_first (id)) {

         result = fetch_config (id, obs);
      }
   }
   else if (_state.client) {

      Config global ("global");

      String id;
      StringContainerIterator it;

      while (IdList.get_next (it, id)) {

         global.add_config (string_to_config ("keys", "value", id));
      }

      String json;
      StreamString out (json);
      if (format_config_to_json (global, out, ConfigStripGlobal, &_state.log)) {

         QByteArray data (json.get_buffer ());

         String DocId ("_all_docs");
         QUrl url (_get_url (DocId));
         url.addQueryItem ("include_docs", "true");

         UInt64 requestId = _state.client->post (url, data);
         if (requestId) {

            _state.get_request (requestId, "fetchMultipleDocuments", DocId, *this);
            result = True;
         }
      }
   }

   return result;
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
dmz::WebServicesModuleQt::fetch_updates (WebServicesObserver &obs, const Int32 Since) {

   return _fetch_changes (obs, Since) ? True : False;
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


dmz::Boolean
dmz::WebServicesModuleQt::stop_realtime_updates (WebServicesObserver &obs) {

   _state.log.error << "NOT IMPLEMENTED YET!!!!" << endl;
}


void
dmz::WebServicesModuleQt::handle_error (const String &Id, const Config &Data) {

   _state.log.error << "Error: " << Id << endl;
   _state.log.error << Data << endl;
}


void
dmz::WebServicesModuleQt::_authenticate (
      QNetworkReply *reply,
      QAuthenticator *authenticator) {

   if (_authenticate (False)) {

      authenticator->setUser (_state.auth.user ());
      authenticator->setPassword (_state.auth.password ());
   }
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

      if (reply->hasRawHeader ("Date")) {

         QString rawDate (reply->rawHeader ("Date"));

         QDateTime dateTime =
            QDateTime::fromString (rawDate, _state.dateFormat.get_buffer ());

         if (dateTime.isValid ()) {

            request->timeStamp = dateTime;
         }
      }

      QByteArray data (reply->readAll ());
      const String JsonData (data.constData ());

      if (reply->error () == QNetworkReply::AuthenticationRequiredError) {

         String reason = qPrintable (reply->errorString ());

         if (request->statusCode == 0) { request->statusCode = reply->error (); }

         request->data = Config ("global");
         request->data.store_attribute ("error", "Reply Error");
         request->data.store_attribute ("reason", reason);

         _handle_error (*request);
      }
      else if (JsonData) {

         if (json_string_to_config (JsonData, request->data)) {

            _handle_reply (*request);
         }
         else {

            request->data = Config ("global");
            request->data.store_attribute ("error", "Parse Error");
            request->data.store_attribute ("reason", "Error parsing json data.");

            _handle_error (*request);
         }
      }
      else {

         String reason = qPrintable (reply->errorString ());

         request->data = Config ("global");
         request->data.store_attribute ("error", "No Data Error");
         request->data.store_attribute ("reason", reason);

         _handle_error (*request);
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

   if (request.Type == "fetchDocument") { _document_fetched (request); }
   else if (request.Type == "fetchMultipleDocuments") { _documents_fetched (request); }
   else if (request.Type == "publishDocument") { _document_published (request); }
   else if (request.Type == "deleteDocument") { _document_deleted (request); }
   else if (request.Type == "fetchChanges") { _changes_fetched (request); }
   else if (request.Type == "postSession") { _session_posted (request); }
   else if (request.Type == "deleteSession") {

      _state.loggedIn = False;
      _state.logoutMsg.send ();
   }
}


void
 dmz::WebServicesModuleQt::_handle_error (RequestStruct &request) {

   request.data.store_attribute ("status-code", String::number (request.statusCode));

   if (request.statusCode == 401 || request.statusCode == 204) {

      request.data.store_attribute ("authentication-required", "true");

      _authenticate ();
   }

_state.log.error << "_handle_error[" << request.Id << "]: " << request.statusCode << endl;
_state.log.error << "reason: " << config_to_string ("reason", request.data) << endl;

   request.obs.handle_error (request.DocId, request.data);
}


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

         UInt64 requestId = _state.client->put (req, data);
         if (requestId) {

#ifdef DMZ_WEBSERVICES_DEBUG
out << ">>>>> [" << requestId << "]  PUT: " << Id << endl;
#endif

            request = _state.get_request (requestId, "publishDocument", Id, obs);
         }
      }
   }

   return request;
}


void
dmz::WebServicesModuleQt::_fetch_session () {

   if (_state.client) {

      const String DocId ("_session");
      QUrl url (_get_root_url (DocId));

      QMap<QString, QString> params;
      params.insert ("name", _state.auth.user ());
      params.insert ("password", _state.auth.password ());

      QNetworkRequest request = _state.client->get_next_request (url);

      request.setHeader (
         QNetworkRequest::ContentTypeHeader,
         "application/x-www-form-urlencoded");

      UInt64 requestId = _state.client->post (request, params);
      if (requestId) {

#ifdef DMZ_WEBSERVICES_DEBUG
out << ">>>>> [" << requestId << "] POST: " << DocId << endl;
#endif

         _state.get_request (requestId, "postSession", DocId, *this);
      }
   }
}


void
dmz::WebServicesModuleQt::_delete_session () {

   if (_state.client) {

      const String DocId ("_session");
      QUrl url (_get_root_url (DocId));

      UInt64 requestId = _state.client->del (url);
      if (requestId) {

#ifdef DMZ_WEBSERVICES_DEBUG
out << ">>>>> [" << requestId << "]  DEL: " << DocId << endl;
#endif

         _state.get_request (requestId, "deleteSession", DocId, *this);
      }
   }
}


dmz::WebServicesModuleQt::RequestStruct *
dmz::WebServicesModuleQt::_fetch_document (const String &Id, WebServicesObserver &obs) {

   RequestStruct *request (0);

   if (Id && _state.client) {

      QUrl url = _get_url (Id);

      UInt64 requestId = _state.client->get (url);
      if (requestId) {

#ifdef DMZ_WEBSERVICES_DEBUG
out << ">>>>> [" << requestId << "]  GET: " << Id << endl;
#endif

         request = _state.get_request (requestId, "fetchDocument", Id, obs);
      }
   }

   return request;
}


dmz::WebServicesModuleQt::RequestStruct *
dmz::WebServicesModuleQt::_delete_document (const String &Id, WebServicesObserver &obs) {

   RequestStruct *request (0);

   if (Id && _state.client) {

      DocStruct *doc = _state.get_doc (Id);
      if (doc && doc->rev) {

         QUrl url = _get_url (Id);
         QNetworkRequest req = _state.client->get_next_request (url);

         req.setRawHeader ("If-Match", doc->rev.get_buffer ());

         UInt64 requestId = _state.client->del (req);
         if (requestId) {

#ifdef DMZ_WEBSERVICES_DEBUG
out << ">>>>> [" << requestId << "]  DEL: " << Id << endl;
#endif

            request = _state.get_request (requestId, "deleteDocument", Id, obs);
         }
      }
   }

   return request;
}


dmz::WebServicesModuleQt::RequestStruct *
dmz::WebServicesModuleQt::_fetch_changes (
      WebServicesObserver &obs,
      const Int32 Since) {

   RequestStruct *request (0);

   if (_state.client) {

      const String Id ("_changes");
      QUrl url = _get_url (Id);
      url.addQueryItem ("since", QString::number (Since));

      UInt64 requestId = _state.client->get (url);
      if (requestId) {

#ifdef DMZ_WEBSERVICES_DEBUG
out << ">>>>> [" << requestId << "]  GET: " << Id << endl;
#endif

         request = _state.get_request (requestId, "fetchChanges", Id, obs);
      }
   }

   return request;
}


void
dmz::WebServicesModuleQt::_document_published (RequestStruct &request) {

#ifdef DMZ_WEBSERVICES_DEBUG
out << "<<<<< [" << request.Id << "]"
    << "[" << request.statusCode << "]"
    << " PUBLISH: " << request.DocId << endl;
#endif

   String docId = config_to_string ("id", request.data);

   if (request.DocId == docId) {

      request.docRev = config_to_string ("rev", request.data);
      request.obs.handle_publish_config (request.DocId, request.docRev);
   }
   else {

      _handle_error (request);
   }
}


void
dmz::WebServicesModuleQt::_document_fetched (RequestStruct &request) {

#ifdef DMZ_WEBSERVICES_DEBUG
out << "<<<<< [" << request.Id << "]"
    << "[" << request.statusCode << "]"
    << " FETCH: " << request.DocId << endl;
#endif

   String docId = config_to_string ("_id", request.data);

   if (request.DocId == docId) {

      request.docRev = config_to_string ("_rev", request.data);

      String runtimeId = config_to_string ("runtime_id", request.data);

      String docType = config_to_string ("type", request.data);

      Config data;
      request.data.lookup_config (docType, data);

      request.obs.handle_fetch_config (
         request.DocId,
         request.docRev,
         data);
   }
   else {

      _handle_error (request);
   }
}


void
dmz::WebServicesModuleQt::_documents_fetched (RequestStruct &request) {

#ifdef DMZ_WEBSERVICES_DEBUG
out << "<<<<< [" << request.Id << "]"
    << "[" << request.statusCode << "]"
    << " FETCH: " << request.DocId << endl;
#endif

   Config rows;
   if (request.data.lookup_all_config ("rows", rows)) {

      ConfigIterator it;
      Config row;
      while (rows.get_next_config (it, row)) {

         Config doc;
         if (row.lookup_config ("doc", doc)) {

            String docId = config_to_string ("_id", doc);
            String docRev = config_to_string ("_rev", doc);
            String runtimeId = config_to_string ("runtime_id", doc);
            String docType = config_to_string ("type", doc);

            Config data;
            doc.lookup_config (docType, data);

            request.obs.handle_fetch_config (docId, docRev, data);

            DocStruct *ds = _state.get_doc (docId);
            if (ds) {

               ds->rev = docRev;
               ds->pending = False;
            }
         }
      }
   }
}


void
dmz::WebServicesModuleQt::_document_deleted (RequestStruct &request) {

#ifdef DMZ_WEBSERVICES_DEBUG
out << "<<<<< [" << request.Id << "]"
    << "[" << request.statusCode << "]"
    << " DELETE: " << request.DocId << endl;
#endif

   String docId = config_to_string ("id", request.data);

   if (request.DocId == docId) {

      request.docRev = config_to_string ("rev", request.data);
      request.obs.handle_delete_config (request.DocId, request.docRev);
   }
   else {

      _handle_error (request);
   }
}


void
dmz::WebServicesModuleQt::_changes_fetched (RequestStruct &request) {

#ifdef DMZ_WEBSERVICES_DEBUG
out << "<<<<< [" << request.Id << "]"
    << "[" << request.statusCode << "]"
    << " CHANGES" << endl;
#endif

   Config updates ("updates");

   String lastSeq = config_to_string ("last_seq", request.data);
   updates.store_attribute ("last_seq", lastSeq);

   ConfigIterator it;
   Config data;

   while (request.data.get_next_config (it, data)) {

      Config item ("config");
      item.store_attribute ("id", config_to_string ("id", data));
      item.store_attribute ("rev", config_to_string ("changes.rev", data));
      item.store_attribute ("deleted", config_to_string ("deleted", data));

      updates.add_config (item);

      if (config_to_boolean ("deleted", data)) {

         DocStruct *doc = _state.documentTable.remove (config_to_string ("id", data));
         if (doc) { delete doc; doc = 0; }
      }
   }

   request.obs.handle_fetch_updates (updates);
}


void
dmz::WebServicesModuleQt::_session_posted (RequestStruct &request) {

#ifdef DMZ_WEBSERVICES_DEBUG
out << "<<<<< [" << request.Id << "]"
    << "[" << request.statusCode << "]"
    << " SESSION" << endl;
#endif

   if (config_to_boolean ("ok", request.data)) {

      _state.loggedIn = True;

      String name = config_to_string ("name", request.data);

      Boolean admin (False);
      Config roles;
      if (request.data.lookup_all_config ("roles", roles)) {

         ConfigIterator it;
         Config data;

         while (!name && roles.get_next_config (it, data)) {

            String role = config_to_string ("value", data);
            if (role == "_admin") { name = "admin"; }
         }
      }

      _state.log.info << "Welcome " << name << " you have logged in." << endl;

      Data data;
      data.store_string (_state.usernameHandle, 0, name);

      _state.loginSuccessMsg.send (&data);
   }
   else {

      _state.loginFailedMsg.send ();
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
            request.obs.handle_realtime_update (Id, Rev, True, lastSeq);
         }
      }
      else {

         DocStruct *doc = _state.get_doc (Id);
         if (doc) {

            if (!doc->pending && (doc->rev != Rev)) {

               doc->rev = Rev;
               request.obs.handle_realtime_update (Id, Rev, False, lastSeq);
            }
         }
      }
   }

   return done;
}


dmz::Boolean
dmz::WebServicesModuleQt::_authenticate (const Boolean GetSession) {

   if (!_state.loggedIn) {

      Data inData;
      Data outData;

      _state.loginRequiredMsg.send (_state.targetHandle, &inData, &outData);
      if (outData) {

         String username;
         outData.lookup_string (_state.usernameHandle, 0, username);

         String password;
         outData.lookup_string (_state.passwordHandle, 0, password);

         if (username && password) {

            _state.auth.setUser (username.get_buffer ());
            _state.auth.setPassword (password.get_buffer ());

            if (GetSession) { _fetch_session (); }
         }
         else {

         }
      }
   }

   return _state.loggedIn;
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

   _state.dateFormat = config_to_string ("date-format.value", local, _state.dateFormat);

   _state.usernameHandle = config_to_named_handle (
      "username.name",
      local,
      WebServicesAttributeUserName,
      context);

   _state.passwordHandle = config_to_named_handle (
      "password.name",
      local,
      WebServicesAttributePassword,
      context);

   _state.loginRequiredMsg = config_create_message (
      "message.login-required",
      local,
      WebServicesLoginRequiredMessageName,
      context);

   _state.loginMsg = config_create_message (
      "message.login",
      local,
      WebServicesLoginMessageName,
      context);

   _state.loginSuccessMsg = config_create_message (
      "message.login-success",
      local,
      WebServicesLoginSuccessMessageName,
      context);

   _state.loginFailedMsg = config_create_message (
      "message.login-failed",
      local,
      WebServicesLoginFailedMessageName,
      context);

   _state.logoutMsg = config_create_message (
      "message.logout",
      local,
      WebServicesLogoutMessageName,
      context);

   subscribe_to_message (_state.loginMsg);
   subscribe_to_message (_state.logoutMsg);

   _state.targetHandle = config_to_named_handle ("login-target.name", local, context);

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
