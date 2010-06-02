#include <dmzForgeConsts.h>
#include <dmzForgeModule.h>
#include "dmzForgePluginPublish.h"
#include <dmzQtConfigRead.h>
#include <dmzRuntimeConfig.h>
#include <dmzRuntimeConfigToTypesBase.h>
#include <dmzRuntimeLog.h>
#include <dmzRuntimePluginFactoryLinkSymbol.h>
#include <dmzRuntimePluginInfo.h>
#include <dmzTypesStringContainer.h>


struct dmz::ForgePluginPublish::State {

   Log log;
   ForgeModule *forgeModule;
   String forgeModuleName;
   String assetId;
   UInt64 requestId;
   
   State (const PluginInfo &Info);
};


dmz::ForgePluginPublish::State::State (const PluginInfo &Info) :
      log (Info),
      forgeModule (0),
      forgeModuleName () {;}


dmz::ForgePluginPublish::ForgePluginPublish (const PluginInfo &Info, Config &local) :
      QFrame (0),
      Plugin (Info),
      TimeSlice (Info, TimeSliceTypeSystemTime, TimeSliceModeSingle, 1.0),
      ForgeObserver (Info),
      _state (*(new State (Info))) {

   _ui.setupUi (this);

   _init (local);
}


dmz::ForgePluginPublish::~ForgePluginPublish () {

   delete &_state;
}


// Plugin Interface
void
dmz::ForgePluginPublish::update_plugin_state (
      const PluginStateEnum State,
      const UInt32 Level) {

   if (State == PluginStateInit) {

   }
   else if (State == PluginStateStart) {

      show ();
   }
   else if (State == PluginStateStop) {

   }
   else if (State == PluginStateShutdown) {

   }
}


void
dmz::ForgePluginPublish::discover_plugin (
      const PluginDiscoverEnum Mode,
      const Plugin *PluginPtr) {

   if (Mode == PluginDiscoverAdd) {

      if (!_state.forgeModule) {

         _state.forgeModule = ForgeModule::cast (PluginPtr, _state.forgeModuleName);
      }
   }
   else if (Mode == PluginDiscoverRemove) {

      if (_state.forgeModule && (_state.forgeModule == ForgeModule::cast (PluginPtr))) {

         _state.forgeModule = 0;
      }
   }
}


void
dmz::ForgePluginPublish::update_time_slice (const Float64 TimeDelta) {

}


void
dmz::ForgePluginPublish::handle_reply (
      const UInt64 RequestId,
      const Int32 ReqeustType,
      const Boolean Error,
      const StringContainer &Results) {

   switch (ReqeustType) {
   
      case ForgeTypeSearch: {
         String assetId;
         while (Results.get_next (assetId)) {

            // _state.log.info << "getting asset: " << assetId << endl;
            // _state.forgeModule->get_asset (assetId, this);
         }
         break;
      }
      
      case ForgeTypeGetAsset:
         break;
         
      case ForgeTypePutAsset:
         break;
      
      case ForgeTypeDeleteAsset:
         break;

      case ForgeTypeGetAssetMedia:
         break;

      case ForgeTypePutAssetMedia:
         break;

      case ForgeTypeAddAssetPreview:
         break;

      default:
         break;
   }

   if (Error) {
      
_state.log.error << "Results: " << Results << endl;
   }
}


void
dmz::ForgePluginPublish::_init (Config &local) {

   setObjectName (get_plugin_name ().get_buffer ());

   qframe_config_read ("frame", local, this);

   _state.forgeModuleName = config_to_string ("module.forge.name", local);
}


extern "C" {

DMZ_PLUGIN_FACTORY_LINK_SYMBOL dmz::Plugin *
create_dmzForgePluginPublish (
      const dmz::PluginInfo &Info,
      dmz::Config &local,
      dmz::Config &global) {

   return new dmz::ForgePluginPublish (Info, local);
}

};
