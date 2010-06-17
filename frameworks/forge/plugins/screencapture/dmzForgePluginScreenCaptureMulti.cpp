#include "dmzForgePluginScreenCaptureMulti.h"
#include <dmzRuntimeConfigToTypesBase.h>
#include <dmzRenderModulePortal.h>
#include <dmzRuntimePluginFactoryLinkSymbol.h>
#include <dmzRuntimePluginInfo.h>
#include <dmzTypesMatrix.h>
#include <dmzTypesConsts.h>


dmz::ForgePluginScreenCaptureMulti::ForgePluginScreenCaptureMulti (
      const PluginInfo &Info,
      Config &local) :
      Plugin (Info),
      TimeSlice (Info),
      MessageObserver (Info),
      _log (Info),
      _convert (Info),
      _listConvert (Info),
      _portal (0),
      _fileIndex (0),
      _maxFiles (16),
      _heading (0),
      _pitch (0) {

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

      if (!_portal) { _portal = RenderModulePortal::cast (PluginPtr); }
   }
   else if (Mode == PluginDiscoverRemove) {

      if (_portal && (_portal == RenderModulePortal::cast (PluginPtr))) { _portal = 0; }
   }
}


// Time Slice Interface
void
dmz::ForgePluginScreenCaptureMulti::update_time_slice (const Float64 TimeDelta) {

   if (_fileIndex > 0) {

      if (_fileIndex > 1) { _heading += TwoPi64 / Float64 (_maxFiles); }
      _log.warn << "_heading: " << _heading << endl;
      _update_portal ();

      String file (_fileRoot);
      file << _fileIndex << ".png";
      _fileList.append (file);

      Data out = _convert.to_data (file);
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

      if (Size == count) {

         Data out = _listConvert.to_data (_fileList);
         _finishedCaptureMsg.send (&out);
         _fileIndex = 0;
         _heading += TwoPi64 / Float64 (_maxFiles);
         _update_portal ();
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

   if (Type == _startCaptureMsg) {

      if (_portal) {

         Matrix ori;
         _portal->get_view (_pos, ori);

         Float64 roll;
         ori.to_euler_angles (_heading, _pitch, roll);
      }

      _fileList.clear ();
      _fileRoot = _convert.to_string (InData);
      if (_fileRoot) { _fileIndex = 1; }
   }
}


void
dmz::ForgePluginScreenCaptureMulti::_update_portal () {

   if (_portal) {

      Matrix hmat (Vector (0.0, 1.0, 0.0), _heading);
      Matrix pmat (Vector (1.0, 0.0, 0.0), _pitch);
      Matrix dmat = hmat * pmat;

      _portal->set_view (_pos, dmat);
   }
}


void
dmz::ForgePluginScreenCaptureMulti::_init (Config &local) {

   RuntimeContext *context = get_plugin_runtime_context ();

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
