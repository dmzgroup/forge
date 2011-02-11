#include <dmzArchiveModule.h>
#include <dmzObjectAttributeMasks.h>
#include <dmzObjectConsts.h>
#include <dmzObjectModule.h>
#include "dmzObjectPluginWebServices.h"
#include <dmzRuntimeConfigToTypesBase.h>
#include <dmzRuntimeData.h>
#include <dmzRuntimeObjectType.h>
#include <dmzRuntimePluginFactoryLinkSymbol.h>
#include <dmzRuntimePluginInfo.h>
#include <dmzTypesMask.h>
#include <dmzTypesMatrix.h>
#include <dmzTypesVector.h>
#include <dmzTypesUUID.h>
#include <dmzWebServicesModule.h>

using namespace dmz;

namespace {

static inline Handle
local_uuid_to_handle (
      const Data &InData,
      const Handle AttrHandle,
      ObjectModule &module,
      const Int32 Offset = 0) {

   UUID uuid;
   InData.lookup_uuid (AttrHandle, Offset, uuid);
   return module.lookup_handle_from_uuid (uuid);
}

static inline Handle
local_attr_name_to_handle (
      const Data &InData,
      const Handle AttrHandle,
      Definitions &defs,
      const Int32 Offset = 0) {

   String attrName;
   InData.lookup_string (AttrHandle, Offset, attrName);
   return defs.create_named_handle (attrName);
}

};


dmz::ObjectPluginWebServices::ObjectPluginWebServices (const PluginInfo &Info, Config &local) :
      Plugin (Info),
      TimeSlice (Info, TimeSliceTypeRuntime, TimeSliceModeRepeating, 2.0),
      WebServicesObserver (Info),
      MessageObserver (Info),
      ObjectObserverUtil (Info, local),
      _log (Info),
      _defs (Info.get_context ()),
      _archiveModule (0),
      _webservices (0),
      _archiveHandle (0),
      _defaultAttrHandle (0) {

   _init (local);
}


dmz::ObjectPluginWebServices::~ObjectPluginWebServices () {

}


// Plugin Interface
void
dmz::ObjectPluginWebServices::update_plugin_state (
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
dmz::ObjectPluginWebServices::discover_plugin (
      const PluginDiscoverEnum Mode,
      const Plugin *PluginPtr) {

   if (Mode == PluginDiscoverAdd) {

      if (!_archiveModule) {

         _archiveModule = ArchiveModule::cast (PluginPtr, _archiveModuleName);
      }

      if (!_webservices) {

         _webservices = WebServicesModule::cast (PluginPtr, _webservicesName);
      }
   }
   else if (Mode == PluginDiscoverRemove) {

      if (_archiveModule && (_archiveModule == ArchiveModule::cast (PluginPtr))) {

         _archiveModule = 0;
      }

      if (_webservices && (_webservices == WebServicesModule::cast (PluginPtr))) {

         _webservices = 0;
      }
   }
}


// TimeSlice Interface
void
dmz::ObjectPluginWebServices::update_time_slice (const Float64 TimeDelta) {

   if (_archiveModule && _webservices) {

      if (_deleteTable.get_count ()) {

         _webservices->delete_configs (_deleteTable, *this);
         _deleteTable.clear ();
      }

      if (_updateTable.get_count ()) {

         Config global = _archiveModule->create_archive (_archiveHandle);
         Config objectList;

         if (global.lookup_all_config ("archive.object", objectList)) {

            ConfigIterator it;
            Config object;

            while (objectList.get_next_config (it, object)) {

               UUID uuid = config_to_string ("uuid", object);
               const String DocId (uuid.to_string ());

               if (!_pendingTable.contains (DocId)) {

                  if (_updateTable.remove (DocId)) {

                     _pendingTable.add (DocId);

                     Config archive ("archive");
                     archive.add_config (object);

                     _webservices->publish_config (DocId, archive, *this);
                  }
               }
            }
         }
      }
   }
}


// WebServicesObserver Interface
void
dmz::ObjectPluginWebServices::config_published (
      const String &Id,
      const Boolean Error,
      const Config &Data) {

   _pendingTable.remove (Id);

   if (Error) {

   }
   else {

      if (_webservices) {

//         _webservices->fetch_config (Id, *this);
      }
   }
}


void
dmz::ObjectPluginWebServices::config_fetched (
   const String &Id,
   const Boolean Error,
   const Config &Data) {

   if (Error) {

   }
   else {

      if (_webservices) {

//         _webservices->publish_config (Id, Data, *this);
      }
   }
}


void
dmz::ObjectPluginWebServices::config_deleted (
      const String &Id,
      const Boolean Error,
      const Config &Data) {

   if (Error) {

   }
   else {

      if (_webservices) {

//         _webservices->publish_config (Id, Data, *this);
      }
   }
}


// Message Observer Interface
void
dmz::ObjectPluginWebServices::receive_message (
      const Message &Msg,
      const UInt32 MessageSendHandle,
      const Handle TargetObserverHandle,
      const Data *InData,
      Data *outData) {

   ObjectModule *objMod (get_object_module ());

   if (objMod && InData) {

   }
}


// Object Observer Interface
void
dmz::ObjectPluginWebServices::create_object (
      const UUID &Identity,
      const Handle ObjectHandle,
      const ObjectType &Type,
      const ObjectLocalityEnum Locality) {

   if (_activeTable.add (ObjectHandle)) {

      _updateTable.add (Identity.to_string ());
   }
}


void
dmz::ObjectPluginWebServices::destroy_object (
      const UUID &Identity,
      const Handle ObjectHandle) {

   if (_activeTable.contains (ObjectHandle)) {

      const String Id (Identity.to_string ());
      _updateTable.remove (Id);
      _deleteTable.add (Id);
   }
}


void
dmz::ObjectPluginWebServices::update_object_uuid (
      const Handle ObjectHandle,
      const UUID &Identity,
      const UUID &PrevIdentity) {

   if (_activeTable.contains (ObjectHandle)) {

//      _deleteTable.add (PrevIdentity.to_string ());
//      _updateTable.add (Identity.to_string ());
   }
}


void
dmz::ObjectPluginWebServices::remove_object_attribute (
      const UUID &Identity,
      const Handle ObjectHandle,
      const Handle AttributeHandle,
      const Mask &AttrMask) {

   _update (Identity, ObjectHandle);
}


void
dmz::ObjectPluginWebServices::update_object_locality (
      const UUID &Identity,
      const Handle ObjectHandle,
      const ObjectLocalityEnum Locality,
      const ObjectLocalityEnum PrevLocality) {

}


void
dmz::ObjectPluginWebServices::link_objects (
      const Handle LinkHandle,
      const Handle AttributeHandle,
      const UUID &SuperIdentity,
      const Handle SuperHandle,
      const UUID &SubIdentity,
      const Handle SubHandle) {

   _update (SuperIdentity, SuperHandle);
   _update (SubIdentity, SubHandle);
}


void
dmz::ObjectPluginWebServices::unlink_objects (
      const Handle LinkHandle,
      const Handle AttributeHandle,
      const UUID &SuperIdentity,
      const Handle SuperHandle,
      const UUID &SubIdentity,
      const Handle SubHandle) {

   _update (SuperIdentity, SuperHandle);
   _update (SubIdentity, SubHandle);
}


void
dmz::ObjectPluginWebServices::update_link_attribute_object (
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

   _update (SuperIdentity, SuperHandle);
   _update (SubIdentity, SubHandle);

   _update (AttributeIdentity, AttributeObjectHandle);

   if (PrevAttributeObjectHandle) {

      _update (PrevAttributeIdentity, PrevAttributeObjectHandle);
   }
}


void
dmz::ObjectPluginWebServices::update_object_counter (
      const UUID &Identity,
      const Handle ObjectHandle,
      const Handle AttributeHandle,
      const Int64 Value,
      const Int64 *PreviousValue) {

   _update (Identity, ObjectHandle);
}


void
dmz::ObjectPluginWebServices::update_object_counter_minimum (
      const UUID &Identity,
      const Handle ObjectHandle,
      const Handle AttributeHandle,
      const Int64 Value,
      const Int64 *PreviousValue) {

   _update (Identity, ObjectHandle);
}


void
dmz::ObjectPluginWebServices::update_object_counter_maximum (
      const UUID &Identity,
      const Handle ObjectHandle,
      const Handle AttributeHandle,
      const Int64 Value,
      const Int64 *PreviousValue) {

   _update (Identity, ObjectHandle);
}


void
dmz::ObjectPluginWebServices::update_object_alternate_type (
      const UUID &Identity,
      const Handle ObjectHandle,
      const Handle AttributeHandle,
      const ObjectType &Value,
      const ObjectType *PreviousValue) {

   _update (Identity, ObjectHandle);
}


void
dmz::ObjectPluginWebServices::update_object_state (
      const UUID &Identity,
      const Handle ObjectHandle,
      const Handle AttributeHandle,
      const Mask &Value,
      const Mask *PreviousValue) {

   _update (Identity, ObjectHandle);
}


void
dmz::ObjectPluginWebServices::update_object_flag (
      const UUID &Identity,
      const Handle ObjectHandle,
      const Handle AttributeHandle,
      const Boolean Value,
      const Boolean *PreviousValue) {

   _update (Identity, ObjectHandle);
}


void
dmz::ObjectPluginWebServices::update_object_time_stamp (
      const UUID &Identity,
      const Handle ObjectHandle,
      const Handle AttributeHandle,
      const Float64 Value,
      const Float64 *PreviousValue) {

   _update (Identity, ObjectHandle);
}


void
dmz::ObjectPluginWebServices::update_object_position (
      const UUID &Identity,
      const Handle ObjectHandle,
      const Handle AttributeHandle,
      const Vector &Value,
      const Vector *PreviousValue) {

   _update (Identity, ObjectHandle);
}


void
dmz::ObjectPluginWebServices::update_object_orientation (
      const UUID &Identity,
      const Handle ObjectHandle,
      const Handle AttributeHandle,
      const Matrix &Value,
      const Matrix *PreviousValue) {

   _update (Identity, ObjectHandle);
}


void
dmz::ObjectPluginWebServices::update_object_velocity (
      const UUID &Identity,
      const Handle ObjectHandle,
      const Handle AttributeHandle,
      const Vector &Value,
      const Vector *PreviousValue) {

   _update (Identity, ObjectHandle);
}


void
dmz::ObjectPluginWebServices::update_object_acceleration (
      const UUID &Identity,
      const Handle ObjectHandle,
      const Handle AttributeHandle,
      const Vector &Value,
      const Vector *PreviousValue) {

   _update (Identity, ObjectHandle);
}


void
dmz::ObjectPluginWebServices::update_object_scale (
      const UUID &Identity,
      const Handle ObjectHandle,
      const Handle AttributeHandle,
      const Vector &Value,
      const Vector *PreviousValue) {

   _update (Identity, ObjectHandle);
}


void
dmz::ObjectPluginWebServices::update_object_vector (
      const UUID &Identity,
      const Handle ObjectHandle,
      const Handle AttributeHandle,
      const Vector &Value,
      const Vector *PreviousValue) {

   _update (Identity, ObjectHandle);
}


void
dmz::ObjectPluginWebServices::update_object_scalar (
      const UUID &Identity,
      const Handle ObjectHandle,
      const Handle AttributeHandle,
      const Float64 Value,
      const Float64 *PreviousValue) {

   _update (Identity, ObjectHandle);
}


void
dmz::ObjectPluginWebServices::update_object_text (
      const UUID &Identity,
      const Handle ObjectHandle,
      const Handle AttributeHandle,
      const String &Value,
      const String *PreviousValue) {

   _update (Identity, ObjectHandle);
}


void
dmz::ObjectPluginWebServices::update_object_data (
      const UUID &Identity,
      const Handle ObjectHandle,
      const Handle AttributeHandle,
      const Data &Value,
      const Data *PreviousValue) {

   _update (Identity, ObjectHandle);
}


// ObjectPluginWebServices Interface
void
dmz::ObjectPluginWebServices::_update (
      const UUID &Identity,
      const Handle ObjectHandle) {

   if (_activeTable.contains (ObjectHandle)) {

      _updateTable.add (Identity.to_string ());
   }
}


void
dmz::ObjectPluginWebServices::_init (Config &local) {

   _webservicesName = config_to_string ("module-name.web-services", local);
   _archiveModuleName = config_to_string ("module-name.archive", local);

   _archiveHandle = _defs.create_named_handle (
      config_to_string ("archive.name", local, ArchiveDefaultName));

   _defaultAttrHandle = _defs.create_named_handle (ObjectAttributeDefaultName);

   activate_global_object_observer ();
}


extern "C" {

DMZ_PLUGIN_FACTORY_LINK_SYMBOL dmz::Plugin *
create_dmzObjectPluginWebServices (
      const dmz::PluginInfo &Info,
      dmz::Config &local,
      dmz::Config &global) {

   return new dmz::ObjectPluginWebServices (Info, local);
}

};
