#ifndef DMZ_WEB_SERVICES_MODULE_QT_DOT_H
#define DMZ_WEB_SERVICES_MODULE_QT_DOT_H

#include <dmzArchiveObserverUtil.h>
#include <dmzObjectObserverUtil.h>
#include <dmzRuntimePlugin.h>
#include <dmzRuntimeTimeSlice.h>
#include <dmzWebServicesModule.h>
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
         public WebServicesModule {

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

         // WebServicesModuleQt Interface
         virtual Boolean is_recording () const;

         virtual Handle start_session (const String &Name);

         virtual Boolean store_record (
            const Message &Type,
            const Handle Target,
            const Data *Record);

         virtual Boolean stop_session (const Handle SessionHandle);
         virtual Boolean abort_session (const Handle SessionHandle);

      protected Q_SLOTS:
         void _reply_aborted (const UInt64 RequestId);

         void _reply_finished (const UInt64 RequestId, QNetworkReply *reply);

         void _handle_reply (
            const UInt64 RequestId,
            const Int32 StatusCode,
            const Config &Global);

         // void _handle_error (
         //    const UInt64 RequestId,
         //    const Int32 StatusCode,
         //    const QString &ErrorMessage);

         // void _reply_error (
         //    const UInt64 RequestId,
         //    const QString &ErrorMessage,
         //    const QNetworkReply::NetworkError Error);

         // void _process_publish_object ();

      protected:
         UInt64 _publish_document (const String &Id, const Config &Data);

         UInt64 _fetch_document (const String &Id);
         UInt64 _fetch_changes (const Int32 Since);

         void _handle_archive (const UInt64 RequestId, const Config &Archive);
         void _handle_session (const UInt64 RequestId, const Config &Session);
         void _handle_changes (const UInt64 RequestId, const Config &Global);

         QUrl _get_url (const String &EndPoint) const;
         
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
