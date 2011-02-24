#ifndef DMZ_WEB_SERVICES_MODULE_QT_DOT_H
#define DMZ_WEB_SERVICES_MODULE_QT_DOT_H

#include <dmzArchiveObserverUtil.h>
#include <dmzObjectObserverUtil.h>
#include <dmzRuntimeMessaging.h>
#include <dmzRuntimePlugin.h>
#include <dmzRuntimeTimeSlice.h>
#include <dmzWebServicesModule.h>
#include <dmzWebServicesObserver.h>
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
         public WebServicesObserver {

      Q_OBJECT

      public:
         WebServicesModuleQt (const PluginInfo &Info, Config &local);
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
         virtual Boolean publish_config (
            const String &Id,
            const Config &Data,
            WebServicesObserver &obs);

         virtual Boolean fetch_config (const String &Id, WebServicesObserver &obs);

         virtual Boolean fetch_configs (
            const StringContainer &IdList,
            WebServicesObserver &obs);

         virtual Boolean delete_config (const String  &Id, WebServicesObserver &obs);

         virtual Boolean delete_configs (
            const StringContainer &IdList,
            WebServicesObserver &obs);

         virtual Boolean get_config_updates (
            WebServicesObserver &obs,
            const Int32 Since,
            const Boolean Heavy);

         virtual Boolean start_realtime_updates (
            WebServicesObserver &obs,
            const Int32 Since);

         // WebServicesObserver Interface
         virtual void config_published (
            const String &Id,
            const Boolean Error,
            const Config &Data) {;}

         virtual void config_fetched (
            const String &Id,
            const Boolean Error,
            const Config &Data) {;}

         virtual void config_deleted (
            const String &Id,
            const Boolean Error,
            const Config &Data) {;}

         virtual void config_updated (
            const String &Id,
            const Boolean Deleted,
            const Int32 Sequence,
            const Config &Data) {;}

         virtual void config_updated (
            const StringContainer &UpdateList,
            const StringContainer &DeleteList,
            const Int32 LastSequence) {;}

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
         struct DocStruct;
         struct RequestStruct;

         void _handle_reply (RequestStruct &request);

//          void _handle_error (
//             const UInt64 RequestId,
//             const Int32 StatusCode,
//             const QString &ErrorMessage);

         RequestStruct *_publish_session (const Handle SessionHandle);

         RequestStruct *_publish_document (
            const String &Id,
            const Config &Data,
            WebServicesObserver &obs);

         void _fetch_session ();
         void _delete_session ();

         RequestStruct *_fetch_document (const String &Id, WebServicesObserver &obs);
         RequestStruct *_delete_document (const String &Id, WebServicesObserver &obs);

         RequestStruct *_fetch_changes (
            WebServicesObserver &obs,
            const Int32 Since,
            const Boolean Heavy);

         void _document_published (RequestStruct &request);
         void _document_fetched (RequestStruct &request);
         void _documents_fetched (RequestStruct &request);
         void _document_deleted (RequestStruct &request);

         void _changes_fetched (RequestStruct &request);
         void _changes_fetched_heavy (RequestStruct &request);

         void _session_posted (RequestStruct &request);

         Boolean _handle_continuous_feed (RequestStruct &request);

         Boolean _authenticate (const Boolean GetSession = True);

         QUrl _get_url (const String &EndPoint) const;
         QUrl _get_root_url (const String &EndPoint) const;

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
