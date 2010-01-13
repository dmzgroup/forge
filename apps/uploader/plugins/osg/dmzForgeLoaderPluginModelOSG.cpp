#include "dmzForgeLoaderPluginModelOSG.h"
#include <dmzInputEventMasks.h>
#include <dmzInputEventMouse.h>
#include <dmzRenderModuleCoreOSG.h>
#include <dmzRenderModulePortal.h>
#include <dmzRuntimePluginFactoryLinkSymbol.h>
#include <dmzRuntimePluginInfo.h>
#include <dmzTypesMatrix.h>
#include <dmzTypesVector.h>

#include <osgDB/ReadFile>


dmz::ForgeLoaderPluginModelOSG::ForgeLoaderPluginModelOSG (
      const PluginInfo &Info,
      Config &local) :
      Plugin (Info),
      MessageObserver (Info),
      InputObserverUtil (Info, local),
      _log (Info),
      _convert (Info),
      _core (0),
      _portal (0) {

   _init (local);
}


dmz::ForgeLoaderPluginModelOSG::~ForgeLoaderPluginModelOSG () {

}


// Plugin Interface
void
dmz::ForgeLoaderPluginModelOSG::update_plugin_state (
      const PluginStateEnum State,
      const UInt32 Level) {

   if (State == PluginStateInit) {

   }
   else if (State == PluginStateStart) {

      Data inData = _convert.to_data ("/Users/barker/DVTE_Models");
      receive_message (_startMsg, 0, 0, &inData, 0);
   }
   else if (State == PluginStateStop) {

   }
   else if (State == PluginStateShutdown) {

   }
}


void
dmz::ForgeLoaderPluginModelOSG::discover_plugin (
      const PluginDiscoverEnum Mode,
      const Plugin *PluginPtr) {

   if (Mode == PluginDiscoverAdd) {

      if (!_core) { _core = RenderModuleCoreOSG::cast (PluginPtr); }
      if (!_portal) { _portal = RenderModulePortal::cast (PluginPtr); }
   }
   else if (Mode == PluginDiscoverRemove) {

      if (_core && (_core == RenderModuleCoreOSG::cast (PluginPtr))) { _core = 0; }
      if (_portal && (_portal == RenderModulePortal::cast (PluginPtr))) { _portal = 0; }
   }
}


// Message Observer Interface
void
dmz::ForgeLoaderPluginModelOSG::receive_message (
      const Message &Type,
      const UInt32 MessageSendHandle,
      const Handle TargetObserverHandle,
      const Data *InData,
      Data *outData) {

   if (Type == _startMsg) {

      _fileList.empty ();

      _path = _convert.to_string (InData);

      if (is_directory (_path)) {

         if (_path.get_char (-1) != '/') { _path << "/"; }

         if (get_file_list (_path, _fileList)) { _send_next_file (); }
      }
   }
   else if (Type == _finishedMsg) { _send_next_file (); }
}


// Input Observer Util Interface
void
dmz::ForgeLoaderPluginModelOSG::receive_mouse_event (
      const Handle Channel,
      const InputEventMouse &Value) {

   if (_portal) {

      if (Value.is_button_pressed (1)) {

         const Float64 PercentX =
            Float64 (Value.get_mouse_delta_x ()) / Float64 (Value.get_window_size_x ());
         const Float64 PercentY =
            Float64 (Value.get_mouse_delta_y ()) / Float64 (Value.get_window_size_y ());
_log.error << PercentX << " " << PercentY << endl;
      }
   }
}


void
dmz::ForgeLoaderPluginModelOSG::_send_next_file () {

   Boolean done = False;

   while (!done) {

      String name;

      if (_fileList.get_next (name)) {

         String path, file, ext;
         split_path_file_ext (name, path, file, ext);

         if (ext == ".ive") {

            if (_core) {

               osg::ref_ptr<osg::Group> scene = _core->get_scene ();

               if (scene.valid ()) {

                  if (_current.valid ()) {

                     scene->removeChild (_current.get ());
                     _current = 0;
                  }

                  _current = osgDB::readNodeFile ((_path + name).get_buffer ());

                  if (_current.valid ()) {

                     scene->addChild (_current);

                     _set_portal ();
                  }
                  else { _log.error << "Unable to read file: " << _path + name << endl; }
               }
            }

            Data out = _convert.to_data (_path + name);

            _processMsg.send (&out);
            done = True;
         }
      }
      else { done = True; }
   }
}


void
dmz::ForgeLoaderPluginModelOSG::_set_portal () {

   if (_portal && _current.valid ()) {

      osg::BoundingSphere bound = _current->computeBound ();
      osg::Vec3 center = bound.center ();
      Float64 radius = bound.radius ();

      Vector dir (1.0, 0.0, 1.0);
      dir.normalize_in_place ();
      Matrix mat (Vector (0.0, 0.0, -1.0), dir);
//      Vector pos (center.x () - (radius * 1.4), center.z (), -center.y () - (radius * 1.4));
//      Vector pos (center.x () - (radius + 1.0), center.z (), -center.y () - (radius + 1.0));
      Vector pos (0.0, 0.0, -radius -2.0);
      mat.transform_vector (pos);
      Vector cpos (center.x (), center.z (), -center.y ());
      pos = cpos - pos;
      _portal->set_view (pos, mat);
   }
}


void
dmz::ForgeLoaderPluginModelOSG::_init (Config &local) {

   RuntimeContext *context = get_plugin_runtime_context ();

   _startMsg = config_create_message (
      "start-processing-files-message.name",
      local,
      "Start_Processing_Files_Message",
      context);

   subscribe_to_message (_startMsg);

   _processMsg = config_create_message (
      "process-file-message.name",
      local,
      "Process_File_Message",
      context);

   _finishedMsg = config_create_message (
      "finished-processing-file-message.name",
      local,
      "Finished_Processing_File_Message",
      context);

   subscribe_to_message (_finishedMsg);

   activate_default_input_channel (InputEventMouseMask);
}


extern "C" {

DMZ_PLUGIN_FACTORY_LINK_SYMBOL dmz::Plugin *
create_dmzForgeLoaderPluginModelOSG (
      const dmz::PluginInfo &Info,
      dmz::Config &local,
      dmz::Config &global) {

   return new dmz::ForgeLoaderPluginModelOSG (Info, local);
}

};
