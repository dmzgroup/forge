#include <dmzArchiveModule.h>
#include <dmzObjectAttributeMasks.h>
#include <dmzObjectConsts.h>
#include <dmzObjectModule.h>
#include "dmzObjectPluginWebServices.h"
#include <dmzRuntimeConfigToTypesBase.h>
#include <dmzRuntimeConfigToMatrix.h>
//#include <dmzRuntimeConfigToStringContainer.h>
#include <dmzRuntimeConfigToVector.h>
//#include <dmzRuntimeConfigWrite.h>
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

//static inline Handle
//local_uuid_to_handle (
//      const Data &InData,
//      const Handle AttrHandle,
//      ObjectModule &module,
//      const Int32 Offset = 0) {

//   UUID uuid;
//   InData.lookup_uuid (AttrHandle, Offset, uuid);
//   return module.lookup_handle_from_uuid (uuid);
//}

//static inline Handle
//local_attr_name_to_handle (
//      const Data &InData,
//      const Handle AttrHandle,
//      Definitions &defs,
//      const Int32 Offset = 0) {

//   String attrName;
//   InData.lookup_string (AttrHandle, Offset, attrName);
//   return defs.create_named_handle (attrName);
//}

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
      _defaultAttrHandle (0),
      _lastSeq (0),
      _inUpdate (False) {

   _init (local);
}


dmz::ObjectPluginWebServices::~ObjectPluginWebServices () {

   _linkTable.empty ();
}


// Plugin Interface
void
dmz::ObjectPluginWebServices::update_plugin_state (
      const PluginStateEnum State,
      const UInt32 Level) {

   if (State == PluginStateInit) {

   }
   else if (State == PluginStateStart) {

      _webservices->start_realtime_updates (*this);
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

      if (_fetchTable.get_count ()) {

      }

      ObjectModule *objMod (get_object_module ());
      if (_linkTable.get_count () && objMod) {

         HashTableHandleIterator it;
         ObjectLinkStruct *links = _linkTable.get_first (it);

         while (links) {

            HashTableStringIterator linkIt;
            LinkStruct *ls = links->table.get_first (linkIt);

            while (ls) {

               if (links->ObjectHandle && ls->subHandle && ls->LinkHandle) {

                  const Handle LinkHandle (objMod->link_objects (
                     ls->LinkHandle,
                     links->ObjectHandle,
                     ls->subHandle));

                  if (LinkHandle) {

                     _log.warn << "LinkHandle: " << LinkHandle << endl;
                  }

//                  if (link->attrObjectId) {

//                  }
               }

               ls = links->table.get_next (linkIt);
            }

            links = _linkTable.get_next (it);
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

      Config object ("object");
      Data.lookup_config ("object", object);

      _inUpdate = True;

      _config_to_object (object);

#if 0
      ObjectModule *objMod (get_object_module ());
      if (objMod) {

         HashTableStringIterator objIt;
         ObjectLinkStruct *current (_linkTable.get_first (objIt));
         while (current) {

            HashTableHandleIterator linkIt;
            LinkGroupStruct *ls (current->table.get_first (linkIt));
            while (ls) {

               HashTableStringIterator subIt;
               LinkStruct *link (ls->table.get_first (subIt));
               while (link) {

                  Handle subHandle = objMod->lookup_handle_from_uuid (link->name);
                  if (subHandle) {

                     const Handle LinkHandle (objMod->link_objects (
                        ls->LinkHandle,
                        current->ObjectHandle,
                        subHandle));

                     if (link->attr) {

                        ObjectLinkStruct *attr (_linkTable.lookup (link->attr));

                        if (attr) {

                           objMod->store_link_attribute_object (
                              LinkHandle,
                              attr->ObjectHandle);
                        }
                     }
                  }

                  link = ls->table.get_next (subIt);
               }

               ls = current->table.get_next (linkIt);
            }

            current = _linkTable.get_next (objIt);
         }
      }

      _linkTable.empty ();
#endif

      _inUpdate = False;

//      if (_archiveModule && _archiveHandle) {

//         Config global ("dmz");
//         global.add_config (Data);

//         _archiveModule->process_archive (_archiveHandle, global);
//      }
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

      StringContainer container;
      container.add (Id);

      _configs_deleted (container);
   }
}


void
dmz::ObjectPluginWebServices::config_updated (
      const String &Id,
      const Boolean Deleted,
      const Int32 Sequence) {

   if (Deleted) {

      StringContainer container;
      container.add (Id);

      _configs_deleted (container);
   }
   else if (_webservices) {

      _webservices->fetch_config (Id, *this);
   }

   _lastSeq = Sequence;
   _log.warn << " lasSeq: " << _lastSeq << endl;
}


void
dmz::ObjectPluginWebServices::config_updated (
      const StringContainer &UpdatedIdList,
      const StringContainer &DeletedIdList,
      const Int32 LastSequence) {

   _configs_deleted (DeletedIdList);

   if (_webservices && UpdatedIdList.get_count ()) {

      _webservices->fetch_configs (UpdatedIdList, *this);
   }

   _lastSeq = LastSequence;
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

      if (!_inUpdate) { _updateTable.add (Identity.to_string ()); }
   }
}


void
dmz::ObjectPluginWebServices::destroy_object (
      const UUID &Identity,
      const Handle ObjectHandle) {

   if (_activeTable.contains (ObjectHandle)) {

      const String Id (Identity.to_string ());
      _updateTable.remove (Id);

      if (_inUpdate) { _deleteTable.remove (Id); }
      else { _deleteTable.add (Id); }
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
dmz::ObjectPluginWebServices::_configs_deleted (const StringContainer &DeleteIdList) {

   ObjectModule *objMod (get_object_module ());
   if(objMod) {

      String id;
      StringContainerIterator it;

      while (DeleteIdList.get_next (it, id)) {

         Handle objectHandle = objMod->lookup_handle_from_uuid (UUID (id));
         if (objectHandle) {

            _inUpdate = True;
            objMod->destroy_object (objectHandle);
            _inUpdate = False;

            _activeTable.remove (objectHandle);
            _pendingTable.remove (id);
            _deleteTable.remove (id);
         }
      }
   }
}

void
dmz::ObjectPluginWebServices::_config_to_object (const Config &Data) {

   ObjectModule *objMod (get_object_module ());

   if (objMod) {

      const UUID ObjUUID (config_to_string ("uuid", Data));
      String objectName (config_to_string ("name", Data));
      if (!objectName) { objectName = ObjUUID.to_string (); }

      const String TypeName (config_to_string ("type", Data));
      const ObjectType Type (TypeName, get_plugin_runtime_context ());

      if (Type) {

         Handle objectHandle (0);

         if (ObjUUID) { objectHandle = objMod->lookup_handle_from_uuid (ObjUUID); }

         if (!objectHandle) { objectHandle = objMod->create_object (Type, ObjectLocal); }

         if (objectHandle) {

            if (ObjUUID) { objMod->store_uuid (objectHandle, ObjUUID); }

            Config attrList;

            if (Data.lookup_all_config ("attributes", attrList)) {

               ConfigIterator it;

               Config attrData;

               Boolean found (attrList.get_first_config (it, attrData));

               while (found) {

                  _store_object_attributes (objectHandle, attrData);
                  found = attrList.get_next_config (it, attrData);
               }
            }

            objMod->activate_object (objectHandle);
         }
         else {

            _log.error << "Unable to create object of type: " << TypeName << endl;
         }
      }
      else if (!Type) {

         _log.error << "Unable to create object of unknown type: " << TypeName << endl;
      }
      else {

         _log.info << "Filtering object with type: " << TypeName << endl;
      }
   }
}


void
dmz::ObjectPluginWebServices::_store_object_attributes (
      const Handle ObjectHandle,
      Config &attrData) {

   ObjectModule *objMod (get_object_module ());
   if (objMod) {

      const String AttributeName (
         config_to_string ("name", attrData, ObjectAttributeDefaultName));

      const Handle AttrHandle (_defs.create_named_handle (AttributeName));

      Config data;
      ConfigIterator it;

      Boolean found (attrData.get_first_config (it, data));

      while (found) {

         const String DataName (data.get_name ().to_lower ());

         if (DataName == "links") {

            StringContainer fetchTable;

            ObjectLinkStruct *links = new ObjectLinkStruct (ObjectHandle);

            if (links && !_linkTable.store (ObjectHandle, links)) {

               delete links; links = 0;
            }

            if (links) {

               Config linkList;

               data.lookup_all_config ("object", linkList);

               Config obj;
               ConfigIterator it;

               Boolean found (linkList.get_first_config (it, obj));

               while (found) {

                  const String Name (config_to_string ("name", obj));

                  LinkStruct *ls = new LinkStruct (Name, AttrHandle);
                  if (ls && Name) {

                     ls->subHandle = objMod->lookup_handle_from_uuid (UUID (Name));

                     if (!ls->subHandle) { fetchTable.add (Name); }

                     if (links->table.store (Name, ls)) {

                        const String AttrName (config_to_string ("attribute", obj));

                        if (AttrName) {

                           ls->attrObjectId = AttrName;

                           ls->attrObjectHandle =
                                 objMod->lookup_handle_from_uuid (UUID (AttrName));

                           if (!ls->attrObjectHandle) { fetchTable.add (AttrName); }
                        }
                     }
                     else { delete ls; ls = 0; }
                  }

                  found = linkList.get_next_config (it, obj);
               }

               if (fetchTable.get_count () && _webservices) {

                  _webservices->fetch_configs (fetchTable, *this);
               }
            }
         }
         else if (DataName == "counter") {

            String valueStr;
            if (data.lookup_attribute ("minimum", valueStr)) {

               objMod->store_counter_minimum (
                  ObjectHandle,
                  AttrHandle,
                  string_to_int64 (valueStr));
            }

            if (data.lookup_attribute ("maximum", valueStr)) {

               objMod->store_counter_maximum (
                  ObjectHandle,
                  AttrHandle,
                  string_to_int64 (valueStr));
            }

            if (data.lookup_attribute ("value", valueStr)) {

                objMod->store_counter (
                   ObjectHandle,
                   AttrHandle,
                   string_to_int64 (valueStr));

                objMod->store_counter_rollover (
                   ObjectHandle,
                   AttrHandle,
                   config_to_boolean ("rollover", data, False));
             }
         }
         else if (DataName == "alttype") {

            const ObjectType Type (
               config_to_string ("value", data),
               get_plugin_runtime_context ());

            objMod->store_alternate_object_type (ObjectHandle, AttrHandle, Type);
         }
         else if (DataName == "state") {

            Mask value;
            _defs.lookup_state (config_to_string ("value", data), value);

            objMod->store_state (ObjectHandle, AttrHandle, value);
         }
         else if (DataName == "flag") {

            objMod->store_flag (
               ObjectHandle,
               AttrHandle,
               config_to_boolean ("value", data));
         }
         else if (DataName == "timestamp") {

            objMod->store_time_stamp (
               ObjectHandle,
               AttrHandle,
               config_to_float64 (data));
         }
         else if (DataName == "position") {

            objMod->store_position (ObjectHandle, AttrHandle, config_to_vector (data));
         }
         else if (DataName == "orientation") {

            objMod->store_orientation (
               ObjectHandle,
               AttrHandle,
               config_to_matrix (data));
         }
         else if (DataName == "euler") {

            const Vector Euler (config_to_vector (data));
            const Matrix Value (Euler.get_x (), Euler.get_y (), Euler.get_z ());

            objMod->store_orientation (ObjectHandle, AttrHandle, Value);
         }
         else if (DataName == "velocity") {

            objMod->store_velocity (ObjectHandle, AttrHandle, config_to_vector (data));
         }
         else if (DataName == "acceleration") {

            objMod->store_acceleration (
               ObjectHandle,
               AttrHandle,
               config_to_vector (data));
         }
         else if (DataName == "scale") {

            objMod->store_scale (ObjectHandle, AttrHandle, config_to_vector (data));
         }
         else if (DataName == "vector") {

            objMod->store_vector (ObjectHandle, AttrHandle, config_to_vector (data));
         }
         else if (DataName == "scalar") {

            Float64 value (config_to_float64 (data));
            objMod->store_scalar (ObjectHandle, AttrHandle, value);
         }
         else if (DataName == "text") {

            objMod->store_text (ObjectHandle, AttrHandle, config_to_string (data));
         }
         else if (DataName == "data") {

            Data value;
            if (config_to_data (data, get_plugin_runtime_context (), value, &_log)) {

               objMod->store_data (ObjectHandle, AttrHandle, value);
            }
         }
         else {

            _log.error << "Unsupported attribute type: " << data.get_name () << endl;
         }

         found = attrData.get_next_config (it, data);
      }
   }
}


void
dmz::ObjectPluginWebServices::_update (
      const UUID &Identity,
      const Handle ObjectHandle) {

   if (!_inUpdate && _activeTable.contains (ObjectHandle)) {

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
