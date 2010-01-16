#ifndef DMZ_FORGE_LOADER_PLUGIN_MODEL_OSG_DOT_H
#define DMZ_FORGE_LOADER_PLUGIN_MODEL_OSG_DOT_H

#include <dmzInputObserverUtil.h>
#include <dmzRuntimeDataConverters.h>
#include <dmzRuntimeLog.h>
#include <dmzRuntimeMessaging.h>
#include <dmzRuntimePlugin.h>
#include <dmzSystemFile.h>
#include <dmzTypesVector.h>

#include <osg/Node>

namespace dmz {

   class RenderModuleCoreOSG;
   class RenderModulePortal;

   class ForgeLoaderPluginModelOSG :
         public Plugin,
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

         RenderModuleCoreOSG *_core;
         RenderModulePortal *_portal;

         Message _startMsg;
         Message _processMsg;
         Message _finishedMsg;

         osg::ref_ptr<osg::Node> _current;
         String _path;
         StringContainer _fileList;

         Vector _center;
         Float64 _radius;
         Float64 _heading;
         Float64 _pitch;

      private:
         ForgeLoaderPluginModelOSG ();
         ForgeLoaderPluginModelOSG (const ForgeLoaderPluginModelOSG &);
         ForgeLoaderPluginModelOSG &operator= (const ForgeLoaderPluginModelOSG &);

   };
};

#endif // DMZ_FORGE_LOADER_PLUGIN_MODEL_OSG_DOT_H
