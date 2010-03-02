#ifndef DMZ_FORGE_LOADER_PLUGIN_MODEL_OSG_DOT_H
#define DMZ_FORGE_LOADER_PLUGIN_MODEL_OSG_DOT_H

#include <dmzInputObserverUtil.h>
#include <dmzRuntimeDataConverterStringContainer.h>
#include <dmzRuntimeDataConverterTypesBase.h>
#include <dmzRuntimeLog.h>
#include <dmzRuntimeMessaging.h>
#include <dmzRuntimePlugin.h>
#include <dmzRuntimeTimeSlice.h>
#include <dmzSystemFile.h>
#include <dmzTypesVector.h>

#include <osg/Node>

namespace dmz {

   class RenderModuleCoreOSG;
   class RenderModulePortal;

   class ForgeLoaderPluginModelOSG :
         public Plugin,
         public TimeSlice,
         public MessageObserver,
         public InputObserverUtil {

      public:
         ForgeLoaderPluginModelOSG (const PluginInfo &Info, Config &local);
         ~ForgeLoaderPluginModelOSG ();

         // Plugin Interface
         virtual void update_plugin_state (
            const PluginStateEnum State,
            const UInt32 Level);

         virtual void discover_plugin (
            const PluginDiscoverEnum Mode,
            const Plugin *PluginPtr);

         // Time Slice Interface
         virtual void update_time_slice (const Float64 TimeDelta);

         // Message Observer Interface
         virtual void receive_message (
            const Message &Type,
            const UInt32 MessageSendHandle,
            const Handle TargetObserverHandle,
            const Data *InData,
            Data *outData);

         // Input Observer Util Interface
         virtual void receive_mouse_event (
            const Handle Channel,
            const InputEventMouse &Value);

      protected:
         void _send_next_file ();
         void _reset_portal ();
         void _set_portal ();
         void _init (Config &local);

         Log _log;
         DataConverterString _convert;
         DataConverterStringContainer _listConvert;

         RenderModuleCoreOSG *_core;
         RenderModulePortal *_portal;

         Message _startMsg;
         Message _processMsg;
         Message _finishedMsg;
         Message _screenMsg;
         Message _doCaptureMsg;
         Message _doneScreenMsg;

         osg::ref_ptr<osg::Node> _current;
         String _path;
         StringContainer _modelList;

         Vector _center;
         Float64 _radius;
         Float64 _heading;
         Float64 _pitch;

         String _fileRoot;
         Int32 _fileIndex;
         Int32 _maxFiles;
         StringContainer _fileList;

      private:
         ForgeLoaderPluginModelOSG ();
         ForgeLoaderPluginModelOSG (const ForgeLoaderPluginModelOSG &);
         ForgeLoaderPluginModelOSG &operator= (const ForgeLoaderPluginModelOSG &);

   };
};

#endif // DMZ_FORGE_LOADER_PLUGIN_MODEL_OSG_DOT_H
