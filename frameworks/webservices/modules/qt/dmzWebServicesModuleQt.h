#ifndef DMZ_WEB_SERVICES_MODULE_QT_DOT_H
#define DMZ_WEB_SERVICES_MODULE_QT_DOT_H

#include <dmzArchiveObserverUtil.h>
#include <dmzObjectObserverUtil.h>
#include <dmzRuntimeMessaging.h>
#include <dmzRuntimePlugin.h>
#include <dmzRuntimeTimeSlice.h>
#include <dmzWebServicesModule.h>
#include <dmzWebServicesCallback.h>
#include <QtCore/QObject>
#include <QtNetwork/QNetworkReply>

class QByteArray;
class QNetworkRequest;
class QString;


namespace dmz {

   class Config;


   class WebServicesModuleQt :
         public QObject,
         public Plugin,
         public TimeSlice,
         public MessageObserver,
         public WebServicesModule,
         public WebServicesCallback {

      Q_OBJECT

      public:
         WebServicesModuleQt (const PluginInfo &Info, Config &local, Config &global);
         ~WebServicesModuleQt ();

         // Plugin Interface
         virtual void update_plugin_state (
            const PluginStateEnum State,
            const UInt32 Level);

         virtual void discover_plugin (
            const PluginDiscoverEnum Mode,
            const Plugin *PluginPtr);

         // TimeSlice Interface
         virtual void update_time_slice (const Float64 TimeDelta);

         // Message Observer Interface
         virtual void receive_message (
            const Message &Type,
            const UInt32 MessageSendHandle,
            const Handle TargetObserverHandle,
            const Data *InData,
            Data *outData);

         // WebServicesModule Interface
         virtual Boolean is_valid_database (const Handle Database);
         virtual String lookup_database_name_from_handle (const Handle Databse);

         virtual Boolean publish_config (
            const Handle Database,
            const String &Id,
            const Config &Data,
            WebServicesCallback &cb);

         virtual Boolean fetch_config (
            const Handle Database,
            const String &Id,
            WebServicesCallback &cb);

         virtual Boolean fetch_configs (
            const Handle Database,
            const StringContainer &List,
            WebServicesCallback &cb);

         virtual Boolean delete_config (
            const Handle Database,
            const String  &Id,
            WebServicesCallback &cb);

         virtual Boolean delete_configs (
            const Handle Database,
            const StringContainer &List,
            WebServicesCallback &cb);

         virtual Boolean fetch_updates (
            const Handle Database,
            WebServicesCallback &cb,
            const Int32 Since);

         virtual Boolean start_realtime_updates (
            const Handle Database,
            WebServicesCallback &cb,
            const Int32 Since);

         virtual Boolean stop_realtime_updates (
            const Handle Database,
            WebServicesCallback &cb);

         // WebServicesCallback Interface
         virtual void handle_error (
            const Handle Database,
            const String &Id,
            const Config &Data);

         virtual void handle_publish_config (
            const Handle Database,
            const String &Id,
            const String &Rev) {;}

         virtual void handle_fetch_config (
            const Handle Database,
            const String &Id,
            const String &Rev,
            const Config &Data) {;}

         virtual void handle_delete_config (
            const Handle Database,
            const String &Id,
            const String &Rev) {;}

         virtual void handle_fetch_updates (
            const Handle Database,
            const Config &Updates) {;}

         virtual void handle_realtime_update (
            const Handle Database,
            const String &Id,
            const String &Rev,
            const Boolean &Deleted,
            const Int32 Sequence) {;}

      Q_SIGNALS:
         void fetch_db_finished ();

      protected Q_SLOTS:
         void _authenticate (QNetworkReply *reply, QAuthenticator *authenticator);

         void _reply_aborted (const UInt64 RequestId);

         void _reply_download_progress (
            const UInt64 RequestId,
            QNetworkReply *reply,
            qint64 bytesReceived,
            qint64 bytesTotal);

         void _reply_finished (const UInt64 RequestId, QNetworkReply *reply);

      protected:
         struct DatabaseStruct;
         struct DocStruct;
         struct RequestStruct;

         void _handle_reply (RequestStruct &request);
         void _handle_error (RequestStruct &request);

         RequestStruct *_publish_document (
            const Handle Database,
            const String &Id,
            const Config &Data,
            WebServicesCallback &cb);

         void _handle_publish_document (RequestStruct &request);

         void _fetch_session ();
         void _handle_fetch_session (RequestStruct &request);

         void _post_session ();
         void _handle_post_session (RequestStruct &request);

         void _delete_session ();

         void _fetch_all_dbs ();
         void _handle_fetch_all_dbs (RequestStruct &request);

         void _fetch_user ();
         void _handle_fetch_user (RequestStruct &reqeust);

         RequestStruct *_fetch_document (
            const Handle Database,
            const String &Id,
            WebServicesCallback &cb);

         void _handle_fetch_document (RequestStruct &request);

         void _handle_fetch_documents (RequestStruct &request);

         RequestStruct *_delete_document (
            const Handle Database,
            const String &Id,
            WebServicesCallback &cb);

         void _handle_delete_document (RequestStruct &request);

         RequestStruct *_fetch_changes (
            const Handle Database,
            WebServicesCallback &cb,
            const Int32 Since);

         void _handle_fetch_changes (RequestStruct &request);

         void _login_user (const Config &User);

         Boolean _handle_continuous_feed (RequestStruct &request);
         Boolean _authenticate (const Boolean GetSession = True);

         QUrl _get_url (const String &Database, const String &EndPoint) const;
         QUrl _get_root_url (const String &EndPoint) const;

         void _init_login (Config &global);
         void _init (Config &local);

      protected:
         struct State;
         State &_state;

      private:
         WebServicesModuleQt ();
         WebServicesModuleQt (const WebServicesModuleQt &);
         WebServicesModuleQt &operator= (const WebServicesModuleQt &);
   };
};

#endif // DMZ_WEB_SERVICES_MODULE_QT_DOT_H
