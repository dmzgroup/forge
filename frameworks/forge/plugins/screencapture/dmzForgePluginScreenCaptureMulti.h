#ifndef DMZ_FORGE_PLUGIN_SCREEN_CAPTURE_MULTI_DOT_H
#define DMZ_FORGE_PLUGIN_SCREEN_CAPTURE_MULTI_DOT_H

#include <dmzRuntimeDataConverterStringContainer.h>
#include <dmzRuntimeDataConverterTypesBase.h>
#include <dmzRuntimeLog.h>
#include <dmzRuntimeMessaging.h>
#include <dmzRuntimePlugin.h>
#include <dmzRuntimeTimeSlice.h>
#include <dmzSystemFile.h>
#include <dmzTypesMatrix.h>
#include <dmzTypesVector.h>


namespace dmz {

   class ObjectModule;
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
         DataConverterHandle _convertHandle;
         DataConverterString _convertString;
         DataConverterStringContainer _convertList;

         ObjectModule *_objMod;
         RenderModulePortal *_portal;

         Handle _defaultAttrHandle;
         Handle _target;

         Message _attachMsg;
         Message _startCaptureMsg;
         Message _doCaptureMsg;
         Message _finishedCaptureMsg;

         String _fileRoot;
         Int32 _fileIndex;
         Int32 _maxFiles;
         Int32 _maxLength;
         StringContainer _fileList;

         Matrix _portalOri;
         Vector _portalPos;

         Vector _targetPos;

         Float64 _heading;

      private:
         ForgePluginScreenCaptureMulti ();
         ForgePluginScreenCaptureMulti (const ForgePluginScreenCaptureMulti &);
         ForgePluginScreenCaptureMulti &operator= (const ForgePluginScreenCaptureMulti &);
   };
};


#endif // DMZ_FORGE_PLUGIN_SCREEN_CAPTURE_MULTI_DOT_H
