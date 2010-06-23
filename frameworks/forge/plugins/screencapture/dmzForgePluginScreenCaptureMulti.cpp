#include "dmzForgePluginScreenCaptureMulti.h"
#include <dmzObjectModule.h>
#include <dmzRenderModulePortal.h>
#include <dmzRuntimeConfigToTypesBase.h>
#include <dmzRuntimeDefinitions.h>
#include <dmzRuntimePluginFactoryLinkSymbol.h>
#include <dmzRuntimePluginInfo.h>
#include <dmzTypesConsts.h>


dmz::ForgePluginScreenCaptureMulti::ForgePluginScreenCaptureMulti (
      const PluginInfo &Info,
      Config &local) :
      Plugin (Info),
      TimeSlice (Info),
      MessageObserver (Info),
      _log (Info),
      _convertHandle (Info),
      _convertString (Info),
      _convertList (Info),
      _objMod (0),
      _portal (0),
      _defaultAttrHandle (0),
      _target (0),
      _fileIndex (0),
      _maxFiles (16),
      _maxLength (2),
      _heading (0) {

   _init (local);
}


dmz::ForgePluginScreenCaptureMulti::~ForgePluginScreenCaptureMulti () {

}


// Plugin Interface
void
dmz::ForgePluginScreenCaptureMulti::update_plugin_state (
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
dmz::ForgePluginScreenCaptureMulti::discover_plugin (
      const PluginDiscoverEnum Mode,
      const Plugin *PluginPtr) {

   if (Mode == PluginDiscoverAdd) {

      if (!_objMod) { _objMod = ObjectModule::cast (PluginPtr); }
      if (!_portal) { _portal = RenderModulePortal::cast (PluginPtr); }
   }
   else if (Mode == PluginDiscoverRemove) {

      if (_objMod && (_objMod == ObjectModule::cast (PluginPtr))) { _objMod = 0; }
      if (_portal && (_portal == RenderModulePortal::cast (PluginPtr))) { _portal = 0; }
   }
}


// Time Slice Interface
void
dmz::ForgePluginScreenCaptureMulti::update_time_slice (const Float64 TimeDelta) {

   if (_fileIndex > 0) {

      if (_fileIndex > 1) {

         _heading += TwoPi64 / Float64 (_maxFiles);
         _update_portal ();
      }

      String count (String::number (_fileIndex));
      if (count.get_length () < _maxLength) {

         count.shift (_maxLength - count.get_length (), '0');
      }

      String file (_fileRoot);
      file << count << ".png";
      _fileList.append (file);

      Data out = _convertString.to_data (file);
      _doCaptureMsg.send (&out);

      _fileIndex++;
      if (_fileIndex > _maxFiles) { _fileIndex = -1; }
   }
   else if (_fileIndex < 0) {

      const Int32 Size (_fileList.get_count ());

      Int32 count (0);
      StringContainerIterator it;
      String file;
      while (_fileList.get_next (it, file)) {

         if (is_valid_path (file)) { count++; }
         else { break; }
      }

      _update_portal ();

      if (Size == count) {

         Data out = _convertList.to_data (_fileList);
         _finishedCaptureMsg.send (&out);
         _fileIndex = 0;
      }
      else {

_log.error << "Not all screen capture images created. Created " << count << " of " << Size << endl;
      }
   }
}


// Message Observer Interface
void
dmz::ForgePluginScreenCaptureMulti::receive_message (
      const Message &Type,
      const UInt32 MessageSendHandle,
      const Handle TargetObserverHandle,
      const Data *InData,
      Data *outData) {

   if (Type == _attachMsg) {

      _target = _convertHandle.to_handle (InData);
   }
   else if (Type == _startCaptureMsg) {

      if (_target && _objMod && _portal) {

         _objMod->lookup_position (_target, _defaultAttrHandle, _targetPos);

         _portal->get_view (_portalPos, _portalOri);

         _heading = 0;

         _fileList.clear ();
         _fileRoot = _convertString.to_string (InData);
         if (_fileRoot) { _fileIndex = 1; }
      }
   }
}


void
dmz::ForgePluginScreenCaptureMulti::_update_portal () {

   if (_portal) {

      Vector dir (_portalPos - _targetPos);

      Matrix hmat (Vector (0.0, 1.0, 0.0), _heading);
      hmat.transform_vector (dir);

      Matrix ori;
      ori.from_vector (-dir);

      _portal->set_view (_targetPos + dir, ori);
   }
}


void
dmz::ForgePluginScreenCaptureMulti::_init (Config &local) {

   RuntimeContext *context = get_plugin_runtime_context ();

   Definitions defs (context, &_log);

   _defaultAttrHandle = defs.create_named_handle (ObjectAttributeDefaultName);

   _attachMsg = config_create_message (
      "attach-message.name",
      local,
      "DMZ_Entity_Attach_Message",
      context);

   subscribe_to_message (_attachMsg);

   _startCaptureMsg = config_create_message (
      "start-screen-capture-message.name",
      local,
      "Start_Screen_Capture_Message",
      context);

   subscribe_to_message (_startCaptureMsg);

   _doCaptureMsg = config_create_message (
      "screen-capture-message.name",
      local,
      "Capture_Render_Screen_Message",
      context);

   _finishedCaptureMsg = config_create_message (
      "finished-screen-capture-message.name",
      local,
      "Finished_Screen_Capture_Message",
      context);

   _maxFiles = config_to_int32 ("max-files.value", local, _maxFiles);
   _maxLength = String::number (_maxFiles).get_length ();
}


extern "C" {

DMZ_PLUGIN_FACTORY_LINK_SYMBOL dmz::Plugin *
create_dmzForgePluginScreenCaptureMulti (
      const dmz::PluginInfo &Info,
      dmz::Config &local,
      dmz::Config &global) {

   return new dmz::ForgePluginScreenCaptureMulti (Info, local);
}

};
