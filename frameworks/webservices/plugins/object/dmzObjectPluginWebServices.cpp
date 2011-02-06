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
      WebServicesObserver (Info),
      MessageObserver (Info),
      ObjectObserverUtil (Info, local),
      _log (Info),
      _defs (Info.get_context ()),
      _webservices (0),
      _recording (False),
      _inDump (False),
      _defaultAttrHandle (0),
      _handleHandle (0),
      _stringHandle (0),
      _uuidHandle (0),
      _valueHandle (0),
      _maskHandle (0) {

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

      if (!_webservices) {

         _webservices = WebServicesModule::cast (PluginPtr, _webservicesName);
         if (_webservices) { _webservices->register_webservices_observer (*this); }
      }
   }
   else if (Mode == PluginDiscoverRemove) {

      if (_webservices && (_webservices == WebServicesModule::cast (PluginPtr))) {

         _webservices->release_webservices_observer (*this);
         _webservices = 0;
      }
   }
}


// WebServicesObserver Interface
void
dmz::ObjectPluginWebServices::start_session () {

   _recording = True;
   _log.warn << "_start_session" << endl;
}


void
dmz::ObjectPluginWebServices::stop_session () {

   _recording = False;
   _log.warn << "_stop_session" << endl;
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

      if (Msg == _createObject) {

_log.error << "---------create object -----------------------" << endl;
         UUID uuid;
         InData->lookup_uuid (_uuidHandle, 0, uuid);

         String typeName;
         InData->lookup_string (_stringHandle, 0, typeName);

         ObjectType type;
         _defs.lookup_object_type (typeName, type);

_log.warn << "type: " << type.get_name () << endl;

         const Handle ObjectHandle (objMod->create_object (type, ObjectLocal));


         if (ObjectHandle) {

            if (objMod->store_uuid (ObjectHandle, uuid)) {

               _log.warn << "uuid: " << uuid.to_string () << endl;
            }

//            objMod->activate_object (ObjectHandle);
         }
      }
      else if (Msg == _activateObject) {

_log.error << "--------------------------------" << endl;
UUID uuid;
InData->lookup_uuid (_uuidHandle, 0, uuid);
_log.warn << "uuid: " << uuid.to_string () << endl;
_log.info << "activate_object: " << local_uuid_to_handle (*InData, _uuidHandle, *objMod) << endl;
_log.error << "--------------------------------" << endl;
         objMod->activate_object (local_uuid_to_handle (*InData, _uuidHandle, *objMod));
      }
      else if (Msg == _destroyObject) {

         objMod->destroy_object (local_uuid_to_handle (*InData, _uuidHandle, *objMod));
      }
      else if (Msg == _storeUUID) {

         const Handle ObjectHandle (local_uuid_to_handle (*InData, _uuidHandle, *objMod));

         UUID uuid;
         InData->lookup_uuid (_uuidHandle, 0, uuid);

         objMod->store_uuid (ObjectHandle, uuid);
      }
      else if (Msg == _removeAttribute) {

         const Handle ObjectHandle (local_uuid_to_handle (*InData, _uuidHandle, *objMod));
         const Handle AttrHandle (
            local_attr_name_to_handle (*InData, _handleHandle, _defs));

         Mask mask;
         InData->lookup_mask (_maskHandle, mask);
         if (mask) { objMod->remove_attribute (ObjectHandle, AttrHandle, mask); }
      }
      else if (Msg == _storeLocality) {

      }
      else if (Msg == _linkObjects) {

         const Handle AttrHandle (
            local_attr_name_to_handle (*InData, _handleHandle, _defs));

         const Handle SuperHandle (local_uuid_to_handle (*InData, _uuidHandle, *objMod));
         const Handle SubHandle (local_uuid_to_handle (*InData, _uuidHandle, *objMod, 1));

         objMod->link_objects (AttrHandle, SuperHandle, SubHandle);
      }
      else if (Msg == _unlinkObjects) {

         const Handle AttrHandle (
            local_attr_name_to_handle (*InData, _handleHandle, _defs));
         const Handle Super (local_uuid_to_handle (*InData, _uuidHandle, *objMod, 0));
         const Handle Sub (local_uuid_to_handle (*InData, _uuidHandle, *objMod, 1));

         objMod->unlink_objects (objMod->lookup_link_handle (AttrHandle, Super, Sub));
      }
      else if (Msg == _storeLinkAttributeObject) {

         const Handle AttrHandle (
            local_attr_name_to_handle (*InData, _handleHandle, _defs));
         const Handle ObjectHandle (local_uuid_to_handle (*InData, _uuidHandle, *objMod));
         const Handle Super (local_uuid_to_handle (*InData, _uuidHandle, *objMod, 1));
         const Handle Sub (local_uuid_to_handle (*InData, _uuidHandle, *objMod, 2));
         const Handle LinkHandle (objMod->lookup_link_handle (AttrHandle, Super, Sub));

         objMod->store_link_attribute_object (LinkHandle, ObjectHandle);
      }
      else if (Msg == _storeCounter) {

         const Handle ObjectHandle (local_uuid_to_handle (*InData, _uuidHandle, *objMod));
         const Handle AttrHandle (
            local_attr_name_to_handle (*InData, _handleHandle, _defs));
         Int64 count (0);
         InData->lookup_int64 (_valueHandle, 0, count);
         objMod->store_counter (ObjectHandle, AttrHandle, count);
      }
      else if (Msg == _storeCounterMin) {

         const Handle ObjectHandle (local_uuid_to_handle (*InData, _uuidHandle, *objMod));
         const Handle AttrHandle (
            local_attr_name_to_handle (*InData, _handleHandle, _defs));
         Int64 count (0);
         InData->lookup_int64 (_valueHandle, 0, count);
         objMod->store_counter_minimum (ObjectHandle, AttrHandle, count);
      }
      else if (Msg == _storeCounterMax) {

         const Handle ObjectHandle (local_uuid_to_handle (*InData, _uuidHandle, *objMod));
         const Handle AttrHandle (
            local_attr_name_to_handle (*InData, _handleHandle, _defs));
         Int64 count (0);
         InData->lookup_int64 (_valueHandle, 0, count);
         objMod->store_counter_maximum (ObjectHandle, AttrHandle, count);
      }
      else if (Msg == _storeType) {

         const Handle ObjectHandle (local_uuid_to_handle (*InData, _uuidHandle, *objMod));
         const Handle AttrHandle (
            local_attr_name_to_handle (*InData, _handleHandle, _defs));
         String typeName;
         InData->lookup_string (_valueHandle, 0, typeName);
         ObjectType type;
         _defs.lookup_object_type (typeName, type);
         objMod->store_alternate_object_type (ObjectHandle, AttrHandle, type);
      }
      else if (Msg == _storeState) {

         const Handle ObjectHandle (local_uuid_to_handle (*InData, _uuidHandle, *objMod));
         const Handle AttrHandle (
            local_attr_name_to_handle (*InData, _handleHandle, _defs));
         Mask state;
         InData->lookup_mask (_valueHandle, state);
         objMod->store_state (ObjectHandle, AttrHandle, state);
      }
      else if (Msg == _storeFlag) {

         const Handle ObjectHandle (local_uuid_to_handle (*InData, _uuidHandle, *objMod));
         const Handle AttrHandle (
            local_attr_name_to_handle (*InData, _handleHandle, _defs));
         Int32 value (0);
         InData->lookup_int32 (_valueHandle, 0, value);
         objMod->store_flag (ObjectHandle, AttrHandle, value != 0);
      }
      else if (Msg == _storeTimeStamp) {

         const Handle ObjectHandle (local_uuid_to_handle (*InData, _uuidHandle, *objMod));
         const Handle AttrHandle (
            local_attr_name_to_handle (*InData, _handleHandle, _defs));
         Float64 value (0.0);
         InData->lookup_float64 (_valueHandle, 0, value);
         objMod->store_time_stamp(ObjectHandle, AttrHandle, value);
      }
      else if (Msg == _storePosition) {

         const Handle ObjectHandle (local_uuid_to_handle (*InData, _uuidHandle, *objMod));
         const Handle AttrHandle (
            local_attr_name_to_handle (*InData, _handleHandle, _defs));
         Vector value;
         InData->lookup_vector (_valueHandle, 0, value);
         objMod->store_position (ObjectHandle, AttrHandle, value);
      }
      else if (Msg == _storeOrientation) {

         const Handle ObjectHandle (local_uuid_to_handle (*InData, _uuidHandle, *objMod));
         const Handle AttrHandle (
            local_attr_name_to_handle (*InData, _handleHandle, _defs));
         Matrix value;
         InData->lookup_matrix (_valueHandle, 0, value);
         objMod->store_orientation (ObjectHandle, AttrHandle, value);
      }
      else if (Msg == _storeVelocity) {

         const Handle ObjectHandle (local_uuid_to_handle (*InData, _uuidHandle, *objMod));
         const Handle AttrHandle (
            local_attr_name_to_handle (*InData, _handleHandle, _defs));
         Vector value;
         InData->lookup_vector (_valueHandle, 0, value);
         objMod->store_velocity (ObjectHandle, AttrHandle, value);
      }
      else if (Msg == _storeAcceleration) {

         const Handle ObjectHandle (local_uuid_to_handle (*InData, _uuidHandle, *objMod));
         const Handle AttrHandle (
            local_attr_name_to_handle (*InData, _handleHandle, _defs));
         Vector value;
         InData->lookup_vector (_valueHandle, 0, value);
         objMod->store_acceleration (ObjectHandle, AttrHandle, value);
      }
      else if (Msg == _storeScale) {

         const Handle ObjectHandle (local_uuid_to_handle (*InData, _uuidHandle, *objMod));
         const Handle AttrHandle (
            local_attr_name_to_handle (*InData, _handleHandle, _defs));
         Vector value;
         InData->lookup_vector (_valueHandle, 0, value);
         objMod->store_scale (ObjectHandle, AttrHandle, value);
      }
      else if (Msg == _storeVector) {

         const Handle ObjectHandle (local_uuid_to_handle (*InData, _uuidHandle, *objMod));
         const Handle AttrHandle (
            local_attr_name_to_handle (*InData, _handleHandle, _defs));
         Vector value;
         InData->lookup_vector (_valueHandle, 0, value);
         objMod->store_vector (ObjectHandle, AttrHandle, value);
      }
      else if (Msg == _storeScalar) {

         const Handle ObjectHandle (local_uuid_to_handle (*InData, _uuidHandle, *objMod));
         const Handle AttrHandle (
            local_attr_name_to_handle (*InData, _handleHandle, _defs));
         Float64 value (0.0);
         InData->lookup_float64 (_valueHandle, 0, value);
         objMod->store_scalar (ObjectHandle, AttrHandle, value);
      }
      else if (Msg == _storeText) {

         const Handle ObjectHandle (local_uuid_to_handle (*InData, _uuidHandle, *objMod));
         const Handle AttrHandle (
            local_attr_name_to_handle (*InData, _handleHandle, _defs));
         String value;
         InData->lookup_string (_valueHandle, 0, value);
         objMod->store_text (ObjectHandle, AttrHandle, value);
      }
      else if (Msg == _storeData) {

         // FIXME FIXME FIXME
         // Need String to Data converter function.
         const Handle ObjectHandle (local_uuid_to_handle (*InData, _uuidHandle, *objMod));
         const Handle AttrHandle (
            local_attr_name_to_handle (*InData, _handleHandle, _defs));
         String value;
         InData->lookup_string (_valueHandle, 0, value);
         Data data;
         // string to data goes here.
         objMod->store_data (ObjectHandle, AttrHandle, data);
      }
   }
}


// Object Observer Interface
void
dmz::ObjectPluginWebServices::create_object (
      const UUID &Identity,
      const Handle ObjectHandle,
      const ObjectType &Type,
      const ObjectLocalityEnum Locality) {

   ObjectModule *objMod (get_object_module ());

    if (!_inDump && _recording && _webservices && objMod) {

       _inDump = True;

       Data data;
       data.store_uuid (_uuidHandle, 0, Identity);
       data.store_string (_stringHandle, 0, Type.get_name ());
       _webservices->store_record (_createObject, get_plugin_handle (), &data);

//       objMod->dump_all_object_attributes (ObjectHandle, *this);

       data.clear ();
       data.store_uuid (_uuidHandle, 0, Identity);
       _webservices->store_record (_activateObject, get_plugin_handle (), &data);

       _inDump = False;
    }
}


void
dmz::ObjectPluginWebServices::destroy_object (
      const UUID &Identity,
      const Handle ObjectHandle) {

    if (_recording && _webservices) {

       Data data;
       data.store_uuid (_uuidHandle, 0, Identity);
       _webservices->store_record (_destroyObject, get_plugin_handle (), &data);
    }
}


void
dmz::ObjectPluginWebServices::update_object_uuid (
      const Handle ObjectHandle,
      const UUID &Identity,
      const UUID &PrevIdentity) {

_log.info << "update_object_uuid: " << ObjectHandle << " : " << Identity.to_string () << endl;

   if (_recording && _webservices) {

      Data data;
      data.store_uuid (_uuidHandle, 0, Identity);
      _webservices->store_record (_storeUUID, get_plugin_handle (), &data);
   }
}


void
dmz::ObjectPluginWebServices::remove_object_attribute (
      const UUID &Identity,
      const Handle ObjectHandle,
      const Handle AttributeHandle,
      const Mask &AttrMask) {

   if (_recording && _webservices) {

      Data data;
      data.store_uuid (_uuidHandle, 0, Identity);
      data.store_string (_handleHandle, 0, _defs.lookup_named_handle_name (AttributeHandle));
      data.store_mask (_maskHandle, AttrMask);
      _webservices->store_record (_removeAttribute, get_plugin_handle (), &data);
   }
}


void
dmz::ObjectPluginWebServices::update_object_locality (
      const UUID &Identity,
      const Handle ObjectHandle,
      const ObjectLocalityEnum Locality,
      const ObjectLocalityEnum PrevLocality) {

   if (_recording) {

   }
}


void
dmz::ObjectPluginWebServices::link_objects (
      const Handle LinkHandle,
      const Handle AttributeHandle,
      const UUID &SuperIdentity,
      const Handle SuperHandle,
      const UUID &SubIdentity,
      const Handle SubHandle) {

   if (_recording && _webservices)  {

      Data data;
      const String AttrName (_defs.lookup_named_handle_name (AttributeHandle));
      data.store_string (_handleHandle, 0, AttrName);
      data.store_uuid (_uuidHandle, 0, SuperIdentity);
      data.store_uuid (_uuidHandle, 1, SubIdentity);
      _webservices->store_record (_linkObjects, get_plugin_handle (), &data);
   }
}


void
dmz::ObjectPluginWebServices::unlink_objects (
      const Handle LinkHandle,
      const Handle AttributeHandle,
      const UUID &SuperIdentity,
      const Handle SuperHandle,
      const UUID &SubIdentity,
      const Handle SubHandle) {

   if (_recording && _webservices)  {

      Data data;
      const String AttrName (_defs.lookup_named_handle_name (AttributeHandle));
      data.store_string (_handleHandle, 0, AttrName);
      data.store_uuid (_uuidHandle, 0, SuperIdentity);
      data.store_uuid (_uuidHandle, 1, SubIdentity);
      _webservices->store_record (_unlinkObjects, get_plugin_handle (), &data);
   }
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

   if (_recording && _webservices) {

      Data data;
      const String AttrName (_defs.lookup_named_handle_name (AttributeHandle));
      data.store_string (_handleHandle, 0, AttrName);
      data.store_uuid (_uuidHandle, 0, AttributeIdentity);
      data.store_uuid (_uuidHandle, 1, SuperIdentity);
      data.store_uuid (_uuidHandle, 2, SubIdentity);
      _webservices->store_record (_storeLinkAttributeObject, get_plugin_handle (), &data);
   }
}


void
dmz::ObjectPluginWebServices::update_object_counter (
      const UUID &Identity,
      const Handle ObjectHandle,
      const Handle AttributeHandle,
      const Int64 Value,
      const Int64 *PreviousValue) {

   if (_recording && _webservices) {

      Data data;
      data.store_uuid (_uuidHandle, 0, Identity);
      const String AttrName (_defs.lookup_named_handle_name (AttributeHandle));
      data.store_string (_handleHandle, 0, AttrName);
      data.store_int64 (_valueHandle, 0, Value);
      _webservices->store_record (_storeCounter, get_plugin_handle (), &data);
   }
}


void
dmz::ObjectPluginWebServices::update_object_counter_minimum (
      const UUID &Identity,
      const Handle ObjectHandle,
      const Handle AttributeHandle,
      const Int64 Value,
      const Int64 *PreviousValue) {

   if (_recording && _webservices) {

      Data data;
      data.store_uuid (_uuidHandle, 0, Identity);
      const String AttrName (_defs.lookup_named_handle_name (AttributeHandle));
      data.store_string (_handleHandle, 0, AttrName);
      data.store_int64 (_valueHandle, 0, Value);
      _webservices->store_record (_storeCounterMin, get_plugin_handle (), &data);
   }
}


void
dmz::ObjectPluginWebServices::update_object_counter_maximum (
      const UUID &Identity,
      const Handle ObjectHandle,
      const Handle AttributeHandle,
      const Int64 Value,
      const Int64 *PreviousValue) {

   if (_recording && _webservices) {

      Data data;
      data.store_uuid (_uuidHandle, 0, Identity);
      const String AttrName (_defs.lookup_named_handle_name (AttributeHandle));
      data.store_string (_handleHandle, 0, AttrName);
      data.store_int64 (_valueHandle, 0, Value);
      _webservices->store_record (_storeCounterMax, get_plugin_handle (), &data);
   }
}


void
dmz::ObjectPluginWebServices::update_object_alternate_type (
      const UUID &Identity,
      const Handle ObjectHandle,
      const Handle AttributeHandle,
      const ObjectType &Value,
      const ObjectType *PreviousValue) {

   if (_recording && _webservices) {

      Data data;
      data.store_uuid (_uuidHandle, 0, Identity);
      const String AttrName (_defs.lookup_named_handle_name (AttributeHandle));
      data.store_string (_handleHandle, 0, AttrName);
      data.store_string (_valueHandle, 0, Value.get_name ());
      _webservices->store_record (_storeType, get_plugin_handle (), &data);
   }
}


void
dmz::ObjectPluginWebServices::update_object_state (
      const UUID &Identity,
      const Handle ObjectHandle,
      const Handle AttributeHandle,
      const Mask &Value,
      const Mask *PreviousValue) {

   if (_recording && _webservices) {

      Data data;
      data.store_uuid (_uuidHandle, 0, Identity);
      const String AttrName (_defs.lookup_named_handle_name (AttributeHandle));
      data.store_string (_handleHandle, 0, AttrName);
      data.store_mask (_valueHandle, Value);
      _webservices->store_record (_storeState, get_plugin_handle (), &data);
   }
}


void
dmz::ObjectPluginWebServices::update_object_flag (
      const UUID &Identity,
      const Handle ObjectHandle,
      const Handle AttributeHandle,
      const Boolean Value,
      const Boolean *PreviousValue) {

   if (_recording && _webservices) {

      Data data;
      data.store_uuid (_uuidHandle, 0, Identity);
      const String AttrName (_defs.lookup_named_handle_name (AttributeHandle));
      data.store_string (_handleHandle, 0, AttrName);
      data.store_int32 (_valueHandle, 0, Value);
      _webservices->store_record (_storeState, get_plugin_handle (), &data);
   }
}


void
dmz::ObjectPluginWebServices::update_object_time_stamp (
      const UUID &Identity,
      const Handle ObjectHandle,
      const Handle AttributeHandle,
      const Float64 Value,
      const Float64 *PreviousValue) {

   if (_recording && _webservices) {

      Data data;
      data.store_uuid (_uuidHandle, 0, Identity);
      const String AttrName (_defs.lookup_named_handle_name (AttributeHandle));
      data.store_string (_handleHandle, 0, AttrName);
      data.store_float64(_valueHandle, 0, Value);
      _webservices->store_record (_storeTimeStamp, get_plugin_handle (), &data);
   }
}


void
dmz::ObjectPluginWebServices::update_object_position (
      const UUID &Identity,
      const Handle ObjectHandle,
      const Handle AttributeHandle,
      const Vector &Value,
      const Vector *PreviousValue) {

   if (_recording && _webservices) {

      Data data;
      data.store_uuid (_uuidHandle, 0, Identity);
      const String AttrName (_defs.lookup_named_handle_name (AttributeHandle));
      data.store_string (_handleHandle, 0, AttrName);
      data.store_vector (_valueHandle, 0, Value);
      _webservices->store_record (_storePosition, get_plugin_handle (), &data);
   }
}


void
dmz::ObjectPluginWebServices::update_object_orientation (
      const UUID &Identity,
      const Handle ObjectHandle,
      const Handle AttributeHandle,
      const Matrix &Value,
      const Matrix *PreviousValue) {

   if (_recording && _webservices) {

      Data data;
      data.store_uuid (_uuidHandle, 0, Identity);
      const String AttrName (_defs.lookup_named_handle_name (AttributeHandle));
      data.store_string (_handleHandle, 0, AttrName);
      data.store_matrix (_valueHandle, 0, Value);
      _webservices->store_record (_storeOrientation, get_plugin_handle (), &data);
   }
}


void
dmz::ObjectPluginWebServices::update_object_velocity (
      const UUID &Identity,
      const Handle ObjectHandle,
      const Handle AttributeHandle,
      const Vector &Value,
      const Vector *PreviousValue) {

   if (_recording && _webservices) {

      Data data;
      data.store_uuid (_uuidHandle, 0, Identity);
      const String AttrName (_defs.lookup_named_handle_name (AttributeHandle));
      data.store_string (_handleHandle, 0, AttrName);
      data.store_vector (_valueHandle, 0, Value);
      _webservices->store_record (_storeVelocity, get_plugin_handle (), &data);
   }
}


void
dmz::ObjectPluginWebServices::update_object_acceleration (
      const UUID &Identity,
      const Handle ObjectHandle,
      const Handle AttributeHandle,
      const Vector &Value,
      const Vector *PreviousValue) {

   if (_recording && _webservices) {

      Data data;
      data.store_uuid (_uuidHandle, 0, Identity);
      const String AttrName (_defs.lookup_named_handle_name (AttributeHandle));
      data.store_string (_handleHandle, 0, AttrName);
      data.store_vector (_valueHandle, 0, Value);
      _webservices->store_record (_storeAcceleration, get_plugin_handle (), &data);
   }
}


void
dmz::ObjectPluginWebServices::update_object_scale (
      const UUID &Identity,
      const Handle ObjectHandle,
      const Handle AttributeHandle,
      const Vector &Value,
      const Vector *PreviousValue) {

   if (_recording && _webservices) {

      Data data;
      data.store_uuid (_uuidHandle, 0, Identity);
      const String AttrName (_defs.lookup_named_handle_name (AttributeHandle));
      data.store_string (_handleHandle, 0, AttrName);
      data.store_vector (_valueHandle, 0, Value);
      _webservices->store_record (_storeScale, get_plugin_handle (), &data);
   }
}


void
dmz::ObjectPluginWebServices::update_object_vector (
      const UUID &Identity,
      const Handle ObjectHandle,
      const Handle AttributeHandle,
      const Vector &Value,
      const Vector *PreviousValue) {

   if (_recording && _webservices) {

      Data data;
      data.store_uuid (_uuidHandle, 0, Identity);
      const String AttrName (_defs.lookup_named_handle_name (AttributeHandle));
      data.store_string (_handleHandle, 0, AttrName);
      data.store_vector (_valueHandle, 0, Value);
      _webservices->store_record (_storeVector, get_plugin_handle (), &data);
   }
}


void
dmz::ObjectPluginWebServices::update_object_scalar (
      const UUID &Identity,
      const Handle ObjectHandle,
      const Handle AttributeHandle,
      const Float64 Value,
      const Float64 *PreviousValue) {

   if (_recording && _webservices) {

      Data data;
      data.store_uuid (_uuidHandle, 0, Identity);
      const String AttrName (_defs.lookup_named_handle_name (AttributeHandle));
      data.store_string (_handleHandle, 0, AttrName);
      data.store_float64 (_valueHandle, 0, Value);
      _webservices->store_record (_storeScalar, get_plugin_handle (), &data);
   }
}


void
dmz::ObjectPluginWebServices::update_object_text (
      const UUID &Identity,
      const Handle ObjectHandle,
      const Handle AttributeHandle,
      const String &Value,
      const String *PreviousValue) {

   if (_recording && _webservices) {

      Data data;
      data.store_uuid (_uuidHandle, 0, Identity);
      const String AttrName (_defs.lookup_named_handle_name (AttributeHandle));
      data.store_string (_handleHandle, 0, AttrName);
      data.store_string (_valueHandle, 0, Value);
      _webservices->store_record (_storeText, get_plugin_handle (), &data);
   }
}


void
dmz::ObjectPluginWebServices::update_object_data (
      const UUID &Identity,
      const Handle ObjectHandle,
      const Handle AttributeHandle,
      const Data &Value,
      const Data *PreviousValue) {

   if (_recording && _webservices) {

      // Convert Data object to string here!
      const String StrValue ("Not Implemented Yet"); // Data.to_string ();

      Data data;
      data.store_uuid (_uuidHandle, 0, Identity);
      const String AttrName (_defs.lookup_named_handle_name (AttributeHandle));
      data.store_string (_handleHandle, 0, AttrName);
      data.store_string (_valueHandle, 0, StrValue);
      _webservices->store_record (_storeData, get_plugin_handle (), &data);
   }
}


// ObjectPluginWebServices Interface
void
dmz::ObjectPluginWebServices::_init (Config &local) {

   _webservicesName = config_to_string ("module-name.web-services", local);

   _defaultAttrHandle = _defs.create_named_handle (ObjectAttributeDefaultName);

   _handleHandle = _defs.create_named_handle ("Handle");
   _stringHandle = _defs.create_named_handle ("String");
   _uuidHandle = _defs.create_named_handle ("UUID");
   _valueHandle = _defs.create_named_handle ("Value");
   _maskHandle = _defs.create_named_handle ("Mask");

   _defs.create_message ("Object_Plugin_WS_Create_Object_Message", _createObject);

   _defs.create_message (
      "Object_Plugin_WS_Activate_Object_Message",
      _activateObject);

   _defs.create_message (
      "Object_Plugin_WS_Destroy_Object_Message",
      _destroyObject);

   _defs.create_message ("Object_Plugin_WS_Store_UUID_Message", _storeUUID);

   _defs.create_message (
      "Object_Plugin_WS_Remove_Attribute_Message",
      _removeAttribute);

   _defs.create_message (
      "Object_Plugin_WS_Store_Locality_Message",
      _storeLocality);

   _defs.create_message ("Object_Plugin_WS_Link_Objects_Message", _linkObjects);

   _defs.create_message (
      "Object_Plugin_WS_Unlink_Objects_Message",
      _unlinkObjects);

   _defs.create_message (
      "Object_Plugin_WS_Store_Link_Attribute_Object_Message",
      _storeLinkAttributeObject);

   _defs.create_message ("Object_Plugin_WS_Store_Counter_Message", _storeCounter);

   _defs.create_message (
      "Object_Plugin_WS_Store_Counter_Minimum_Message",
      _storeCounterMin);

   _defs.create_message (
      "Object_Plugin_WS_Store_Counter_Maximum_Message",
      _storeCounterMax);

   _defs.create_message ("Object_Plugin_WS_Store_Type_Message", _storeType);
   _defs.create_message ("Object_Plugin_WS_Store_State_Message", _storeState);
   _defs.create_message ("Object_Plugin_WS_Store_Flag_Message", _storeFlag);

   _defs.create_message (
      "Object_Plugin_WS_Store_Time_Stamp_Message",
      _storeTimeStamp);

   _defs.create_message (
      "Object_Plugin_WS_Store_Position_Message",
      _storePosition);

   _defs.create_message (
      "Object_Plugin_WS_Store_Orientation_Message",
      _storeOrientation);

   _defs.create_message (
      "Object_Plugin_WS_Store_Velocity_Message",
      _storeVelocity);

   _defs.create_message (
      "Object_Plugin_WS_Store_Acceleration_Message",
      _storeAcceleration);

   _defs.create_message ("Object_Plugin_WS_Store_Scale_Message", _storeScale);
   _defs.create_message ("Object_Plugin_WS_Store_Vector_Message", _storeVector);
   _defs.create_message ("Object_Plugin_WS_Store_Scalar_Message", _storeScalar);
   _defs.create_message ("Object_Plugin_WS_Store_Text_Message", _storeText);
   _defs.create_message ("Object_Plugin_WS_Store_Data_Message", _storeData);

   subscribe_to_message (_createObject);
   subscribe_to_message (_activateObject);
   subscribe_to_message (_destroyObject);
   subscribe_to_message (_storeUUID);
   subscribe_to_message (_removeAttribute);
   subscribe_to_message (_storeLocality);
   subscribe_to_message (_linkObjects);
   subscribe_to_message (_unlinkObjects);
   subscribe_to_message (_storeLinkAttributeObject);
   subscribe_to_message (_storeCounter);
   subscribe_to_message (_storeCounterMin);
   subscribe_to_message (_storeCounterMax);
   subscribe_to_message (_storeType);
   subscribe_to_message (_storeState);
   subscribe_to_message (_storeFlag);
   subscribe_to_message (_storeTimeStamp);
   subscribe_to_message (_storePosition);
   subscribe_to_message (_storeOrientation);
   subscribe_to_message (_storeVelocity);
   subscribe_to_message (_storeAcceleration);
   subscribe_to_message (_storeScale);
   subscribe_to_message (_storeVector);
   subscribe_to_message (_storeScalar);
   subscribe_to_message (_storeText);
   subscribe_to_message (_storeData);

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
