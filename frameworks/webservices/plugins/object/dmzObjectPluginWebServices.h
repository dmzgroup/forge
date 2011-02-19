#ifndef DMZ_OBJECT_PLUGIN_WEB_SERVICES_DOT_H
#define DMZ_OBJECT_PLUGIN_WEB_SERVICES_DOT_H

#include <dmzObjectObserverUtil.h>
#include <dmzRuntimeDefinitions.h>
#include <dmzRuntimeLog.h>
#include <dmzRuntimeMessaging.h>
#include <dmzRuntimeObjectType.h>
#include <dmzRuntimePlugin.h>
#include <dmzRuntimeTimeSlice.h>
#include <dmzTypesDeleteListTemplate.h>
#include <dmzTypesHashTableHandleTemplate.h>
#include <dmzTypesHashTableStringTemplate.h>
#include <dmzTypesHashTableUUIDTemplate.h>
#include <dmzWebServicesObserver.h>


namespace dmz {

   class WebServicesModule;

   class ObjectPluginWebServices :
         public Plugin,
         public TimeSlice,
         public WebServicesObserver,
         public MessageObserver,
         public ObjectObserverUtil {

      public:
         ObjectPluginWebServices (const PluginInfo &Info, Config &local);
         ~ObjectPluginWebServices ();

         // Plugin Interface
         virtual void update_plugin_state (
            const PluginStateEnum State,
            const UInt32 Level);

         virtual void discover_plugin (
            const PluginDiscoverEnum Mode,
            const Plugin *PluginPtr);

         // TimeSlice Interface
         virtual void update_time_slice (const Float64 TimeDelta);

         // WebServicesObserver Interface
         virtual void config_published (
            const String &Id,
            const Boolean Error,
            const Config &Data);

         virtual void config_fetched (
            const String &Id,
            const Boolean Error,
            const Config &Data);

         virtual void config_deleted (
            const String &Id,
            const Boolean Error,
            const Config &Data);

         virtual void config_updated (
            const String &Id,
            const Boolean Deleted,
            const Int32 Sequence,
            const Config &Data);

         virtual void config_updated (
            const StringContainer &UpdatedIdList,
            const StringContainer &DeletedIdList,
            const Int32 LastSequence);

         // Message Observer Interface
         virtual void receive_message (
            const Message &Type,
            const UInt32 MessageSendHandle,
            const Handle TargetObserverHandle,
            const Data *InData,
            Data *outData);

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

      protected:
         struct FilterAttrStruct {

            const String Name;
            const Mask Attr;
            FilterAttrStruct *next;

            FilterAttrStruct (const String TheName, const Mask TheAttr) :
                  Name (TheName),
                  Attr (TheAttr),
                  next (0) {;}

            ~FilterAttrStruct () { delete_list (next);  }
         };

         struct FilterStruct {

            FilterStruct *next;
            ObjectTypeSet inTypes;
            ObjectTypeSet exTypes;
            HashTableHandleTemplate<Mask> attrTable;
            HashTableHandleTemplate<Mask> stateTable;
            FilterAttrStruct *list;

            FilterStruct () :
                  next (0),
                  list (0) {;}

            ~FilterStruct () {

               delete_list (next);
               delete_list (list);
               attrTable.empty ();
               stateTable.empty ();
            }
         };

         struct LinkStruct {

            Handle AttrHandle;
            Handle SuperHandle;
            Handle linkHandle;
            String subName;
            String attrObjectName;

            LinkStruct (const Handle TheAttrHandle, const Handle TheSuperHandle) :
               AttrHandle (TheAttrHandle),
               SuperHandle (TheSuperHandle),
               linkHandle (0) {;}
         };

         struct ObjectLinkStruct {

            const String SubName;
            HashTableHandleTemplate<LinkStruct> inLinks;

            ObjectLinkStruct (const String &Name) : SubName (Name) {;}
            ~ObjectLinkStruct () { inLinks.empty (); }
         };

         Boolean _publish (const Handle ObjectHandle);
         Boolean _fetch (const String &Id);

         void _link_to_sub (ObjectLinkStruct &objLink);

         void _configs_deleted (const StringContainer &DeleteIdList);

         Config _archive_object (const Handle ObjectHandle);

         Boolean _get_attr_config (const Handle AttrHandle, Config &config);

         void _config_to_object (const Config &Data);

         void _config_to_object_attributes (
            const Handle ObjectHandle,
            const Config &Data);

         Boolean _add_sub_link (
            const Handle AttrHandle,
            const Handle SuperHandle,
            const String &SubName,
            const String &AttrObjName);

         Boolean _handle_type (const ObjectType &Type);

         Boolean _handle_attribute (
            const Handle AttrHandle,
            const Mask &AttrMask);

         Mask _filter_state (const Handle AttrHandle, const Mask &Value);

         Boolean _active (const Handle ObjectHandle);
         Boolean _update (const Handle ObjectHandle);

         void _init_filter_list (Config &local);
         void _init_object_type_filter (Config &objects, FilterStruct &filter);
         void _init_attribute_filter (Config &attrConfig, FilterStruct &filter);
         void _init_state_filter (Config &stateConfig, FilterStruct &filter);
         void _init (Config &local);

         Log _log;
         Definitions _defs;

         WebServicesModule *_webservices;
         String _webservicesName;

         FilterStruct *_filterList;

         Handle _defaultHandle;
         HandleContainer _activeTable;
         HandleContainer _publishTable;
         StringContainer _fetchTable;
         StringContainer _deleteTable;
         StringContainer _pendingPublishTable;
         StringContainer _pendingFetchTable;

         Message _loginSuccessMsg;
         Message _loginFailedMsg;
         Message _logoutMsg;

         Config _currentConfig;
         HashTableHandleTemplate<Config> _configTable;

         HashTableStringTemplate<ObjectLinkStruct> _objectLinkTable;

         Int32 _lastSeq;
         Boolean _inDump;
         Boolean _inUpdate;
         Float64 _publishRate;
         Float64 _publishDelta;

      private:
         ObjectPluginWebServices ();
         ObjectPluginWebServices (const ObjectPluginWebServices &);
         ObjectPluginWebServices &operator= (const ObjectPluginWebServices &);
   };
};

#endif // DMZ_OBJECT_PLUGIN_WEB_SERVICES_DOT_H
