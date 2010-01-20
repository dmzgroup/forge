#include "dmzForgeLoaderPluginModelOSG.h"
#include <dmzInputEventMasks.h>
#include <dmzInputEventMouse.h>
#include <dmzRenderModuleCoreOSG.h>
#include <dmzRenderModulePortal.h>
#include <dmzRuntimePluginFactoryLinkSymbol.h>
#include <dmzRuntimePluginInfo.h>
#include <dmzTypesMatrix.h>
#include <dmzTypesVector.h>
#include <dmzTypesConsts.h>

#include <osgDB/ReadFile>

dmz::ForgeLoaderPluginModelOSG::ForgeLoaderPluginModelOSG (
      const PluginInfo &Info,
      Config &local) :
      Plugin (Info),
      TimeSlice (Info),
      MessageObserver (Info),
      InputObserverUtil (Info, local),
      _log (Info),
      _convert (Info),
      _listConvert (Info),
      _core (0),
      _portal (0),
      _radius (0.0),
      _heading (0.0),
      _pitch (0.0),
      _fileIndex (0) {

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


// Time Slice Interface
void
dmz::ForgeLoaderPluginModelOSG::update_time_slice (const Float64 TimeDelta) {

   if (_fileIndex > 0) {

      if (_fileIndex > 1) { _heading += TwoPi64 / 8.0; }
      _set_portal ();
      String file (_fileRoot);
      file << _fileIndex << ".png";
      _fileList.append (file);
      Data out = _convert.to_data (file);
      _doCaptureMsg.send (&out);
      _fileIndex++;
      if (_fileIndex > 8) { _fileIndex = -1; }
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
         _doneScreenMsg.send (&out);
         _fileIndex = 0;
      }
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

      _modelList.clear ();

      _path = _convert.to_string (InData);

      if (is_directory (_path)) {

         if (_path.get_char (-1) != '/') { _path << "/"; }

         if (get_file_list (_path, _modelList)) { _send_next_file (); }
      }
   }
   else if (Type == _finishedMsg) { _send_next_file (); }
   else if (Type == _screenMsg) {

      _fileList.clear ();
      _fileRoot = _convert.to_string (InData);
      if (_fileRoot) { _fileIndex = 1; }
   }
}


// Input Observer Util Interface
void
dmz::ForgeLoaderPluginModelOSG::receive_mouse_event (
      const Handle Channel,
      const InputEventMouse &Value) {

   if (_portal) {

      Boolean setPortal = False;

      const Float64 SizeY = Float64 (Value.get_window_size_y ());

      if (Value.is_button_pressed (1)) {

         const Float64 SizeX = Float64 (Value.get_window_size_x ());

         const Float64 PercentX =
            Float64 (Value.get_mouse_delta_x ()) / (SizeX > 0.0 ? SizeX : 0.0);

         const Float64 PercentY =
            Float64 (Value.get_mouse_delta_y ()) / (SizeY > 0.0 ? SizeY : 0.0);

         _heading -= PercentX * Pi64;
         _pitch -= PercentY * Pi64;

         if (_heading < 0) { _heading += TwoPi64; }
         else if (_heading > TwoPi64) { _heading -= TwoPi64; }

         if (_pitch < 0) { _pitch += TwoPi64; }
         else if (_pitch > TwoPi64) { _pitch -= TwoPi64; }

         setPortal = True;
      }

      if (Value.is_button_pressed (3)) {

         const Float64 PercentY =
            Float64 (Value.get_mouse_delta_y ()) / (SizeY > 0.0 ? SizeY : 0.0);

         _radius += PercentY * 5.0;

         setPortal = True;
      }

      const Int32 ZoomInt = Value.get_scroll_delta_y ();

      if (ZoomInt != 0) {

         _radius += Float64 (ZoomInt) * 0.2;
         setPortal = True;
      }

      if (setPortal) { _set_portal (); }
   }
}


void
dmz::ForgeLoaderPluginModelOSG::_send_next_file () {

   Boolean done = False;

   while (!done) {

      String name;

      if (_modelList.get_next (name)) {

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

                     _reset_portal ();
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
dmz::ForgeLoaderPluginModelOSG::_reset_portal () {

   if (_current.valid ()) {

      osg::BoundingSphere bound = _current->computeBound ();
      osg::Vec3 center = bound.center ();
      _center.set_xyz (center.x (), center.z (), -center.y ());
      _radius = bound.radius () + 2.0;
      _heading = 1.25 * Pi64;
      _pitch = -Pi64 / 8.0;
      _set_portal ();
   }
}


void
dmz::ForgeLoaderPluginModelOSG::_set_portal () {

   if (_portal) {

      Matrix hmat (Vector (0.0, 1.0, 0.0), _heading);
      Matrix pmat (Vector (1.0, 0.0, 0.0), _pitch);
      Matrix dmat = hmat * pmat;
      Vector dir (0.0, 0.0, _radius);
      dmat.transform_vector (dir);

      _portal->set_view (_center + dir, dmat);
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

  _screenMsg = config_create_message (
     "start-screen-capture-message.name",
     local,
     "Start_Screen_Capture_Message",
     context);

   subscribe_to_message (_screenMsg);

   _doCaptureMsg = config_create_message (
      "screen-capture-message.name",
      local,
      "Capture_Render_Screen_Message",
      context);

  _doneScreenMsg = config_create_message (
     "finished-screen-capture-message.name",
     local,
     "Finished_Screen_Capture_Message",
     context);

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
