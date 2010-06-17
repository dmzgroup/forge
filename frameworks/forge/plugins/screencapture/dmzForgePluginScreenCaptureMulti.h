#ifndef DMZ_FORGE_PLUGIN_SCREEN_CAPTURE_MULTI_DOT_H
#define DMZ_FORGE_PLUGIN_SCREEN_CAPTURE_MULTI_DOT_H

#include <dmzRuntimeDataConverterStringContainer.h>
#include <dmzRuntimeDataConverterTypesBase.h>
#include <dmzRuntimeLog.h>
#include <dmzRuntimeMessaging.h>
#include <dmzRuntimePlugin.h>
#include <dmzRuntimeTimeSlice.h>
#include <dmzSystemFile.h>
#include <dmzTypesVector.h>


namespace dmz {

   class RenderModulePortal;

   class ForgePluginScreenCaptureMulti :
         public Plugin,
         public TimeSlice,
         public MessageObserver {

      public:
         ForgePluginScreenCaptureMulti (const PluginInfo &Info, Config &local);
         ~ForgePluginScreenCaptureMulti ();

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

      protected:
         void _update_portal ();
         void _init (Config &local);

         Log _log;
         DataConverterString _convert;
         DataConverterStringContainer _listConvert;

         RenderModulePortal *_portal;

         Message _startCaptureMsg;
         Message _doCaptureMsg;
         Message _finishedCaptureMsg;

         String _fileRoot;
         Int32 _fileIndex;
         Int32 _maxFiles;
         StringContainer _fileList;

         Float64 _heading;
         Float64 _pitch;
         Vector _pos;

      private:
         ForgePluginScreenCaptureMulti ();
         ForgePluginScreenCaptureMulti (const ForgePluginScreenCaptureMulti &);
         ForgePluginScreenCaptureMulti &operator= (const ForgePluginScreenCaptureMulti &);
   };
};


#endif // DMZ_FORGE_PLUGIN_SCREEN_CAPTURE_MULTI_DOT_H
