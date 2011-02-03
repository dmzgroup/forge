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
class QUrl;


namespace dmz {

   class Config;


   class WebServicesModuleQt :
         public QObject,
         public Plugin,
         public TimeSlice,
         public ArchiveObserverUtil,
         public ObjectObserverUtil,
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

         // ArchiveObserver Interface
         virtual void pre_process_archive (
            const Handle ArchiveHandle,
            const Int32 Version);

         virtual void process_archive (
            const Handle ArchiveHandle,
            const Int32 Version,
            Config &local,
            Config &global);

         virtual void post_process_archive (
            const Handle ArchiveHandle,
            const Int32 Version);

         // Object Observer Interface
         virtual void create_object (
            const UUID &Identity,
            const Handle ObjectHandle,
            const ObjectType &Type,
            const ObjectLocalityEnum Locality);

         virtual void destroy_object (const UUID &Identity, const Handle ObjectHandle);

         virtual void update_object_uuid (
            const Handle ObjectHandle,
            const UUID &Identity,
            const UUID &PrevIdentity);

         virtual void remove_object_attribute (
            const UUID &Identity,
            const Handle ObjectHandle,
            const Handle AttributeHandle,
            const Mask &AttrMask);

         virtual void update_object_locality (
            const UUID &Identity,
            const Handle ObjectHandle,
            const ObjectLocalityEnum Locality,
            const ObjectLocalityEnum PrevLocality);

         virtual void link_objects (
            const Handle LinkHandle,
            const Handle AttributeHandle,
            const UUID &SuperIdentity,
            const Handle SuperHandle,
            const UUID &SubIdentity,
            const Handle SubHandle);

         virtual void unlink_objects (
            const Handle LinkHandle,
            const Handle AttributeHandle,
            const UUID &SuperIdentity,
            const Handle SuperHandle,
            const UUID &SubIdentity,
            const Handle SubHandle);

         virtual void update_link_attribute_object (
            const Handle LinkHandle,
            const Handle AttributeHandle,
            const UUID &SuperIdentity,
            const Handle SuperHandle,
            const UUID &SubIdentity,
            const Handle SubHandle,
            const UUID &AttributeIdentity,
            const Handle AttributeObjectHandle,
            const UUID &PrevAttributeIdentity,
            const Handle PrevAttributeObjectHandle);

         virtual void update_object_counter (
            const UUID &Identity,
            const Handle ObjectHandle,
            const Handle AttributeHandle,
            const Int64 Value,
            const Int64 *PreviousValue);

         virtual void update_object_counter_minimum (
            const UUID &Identity,
            const Handle ObjectHandle,
            const Handle AttributeHandle,
            const Int64 Value,
            const Int64 *PreviousValue);

         virtual void update_object_counter_maximum (
            const UUID &Identity,
            const Handle ObjectHandle,
            const Handle AttributeHandle,
            const Int64 Value,
            const Int64 *PreviousValue);

         virtual void update_object_alternate_type (
            const UUID &Identity,
            const Handle ObjectHandle,
            const Handle AttributeHandle,
            const ObjectType &Value,
            const ObjectType *PreviousValue);

         virtual void update_object_state (
            const UUID &Identity,
            const Handle ObjectHandle,
            const Handle AttributeHandle,
            const Mask &Value,
            const Mask *PreviousValue);

         virtual void update_object_flag (
            const UUID &Identity,
            const Handle ObjectHandle,
            const Handle AttributeHandle,
            const Boolean Value,
            const Boolean *PreviousValue);

         virtual void update_object_time_stamp (
            const UUID &Identity,
            const Handle ObjectHandle,
            const Handle AttributeHandle,
            const Float64 Value,
            const Float64 *PreviousValue);

         virtual void update_object_position (
            const UUID &Identity,
            const Handle ObjectHandle,
            const Handle AttributeHandle,
            const Vector &Value,
            const Vector *PreviousValue);

         virtual void update_object_orientation (
            const UUID &Identity,
            const Handle ObjectHandle,
            const Handle AttributeHandle,
            const Matrix &Value,
            const Matrix *PreviousValue);

         virtual void update_object_velocity (
            const UUID &Identity,
            const Handle ObjectHandle,
            const Handle AttributeHandle,
            const Vector &Value,
            const Vector *PreviousValue);

         virtual void update_object_acceleration (
            const UUID &Identity,
            const Handle ObjectHandle,
            const Handle AttributeHandle,
            const Vector &Value,
            const Vector *PreviousValue);

         virtual void update_object_scale (
            const UUID &Identity,
            const Handle ObjectHandle,
            const Handle AttributeHandle,
            const Vector &Value,
            const Vector *PreviousValue);

         virtual void update_object_vector (
            const UUID &Identity,
            const Handle ObjectHandle,
            const Handle AttributeHandle,
            const Vector &Value,
            const Vector *PreviousValue);

         virtual void update_object_scalar (
            const UUID &Identity,
            const Handle ObjectHandle,
            const Handle AttributeHandle,
            const Float64 Value,
            const Float64 *PreviousValue);

         virtual void update_object_text (
            const UUID &Identity,
            const Handle ObjectHandle,
            const Handle AttributeHandle,
            const String &Value,
            const String *PreviousValue);

         virtual void update_object_data (
            const UUID &Identity,
            const Handle ObjectHandle,
            const Handle AttributeHandle,
            const Data &Value,
            const Data *PreviousValue);

         // WebServicesModule Interface

         virtual UInt64 put_object (
            const UUID &Identity,
            WebServicesObserver *observer);

         virtual UInt64 get_object (
            const UUID &Identity,
            WebServicesObserver *observer);

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
         // WebServicesModuleQt Interface
         UInt64 _publish_document (const String &Id, const Config &Data);

         UInt64 _fetch_document (const String &Id);
         UInt64 _fetch_changes (const Int32 Since);

         void _handle_archive (const UInt64 RequestId, const Config &Archive);
         void _handle_changes (const UInt64 RequestId, const Config &Global);

         void _add_update (const Handle &ObjectHandle);

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
