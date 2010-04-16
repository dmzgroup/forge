#include <dmzForgeModule.h>
#include "dmzForgeQtPluginClient.h"
#include <dmzRuntimeConfig.h>
#include <dmzRuntimeConfigToTypesBase.h>
#include <dmzRuntimeLog.h>
#include <dmzRuntimePluginFactoryLinkSymbol.h>
#include <dmzRuntimePluginInfo.h>
#include <dmzTypesStringContainer.h>


struct dmz::ForgeQtPluginClient::State {

   Log log;
   ForgeModule *forgeModule;
   String forgeModuleName;
   String assetId;
   UInt64 requestId;
   
   State (const PluginInfo &Info);
};


dmz::ForgeQtPluginClient::State::State (const PluginInfo &Info) :
      log (Info),
      forgeModule (0),
      forgeModuleName () {;}


dmz::ForgeQtPluginClient::ForgeQtPluginClient (const PluginInfo &Info, Config &local) :
      QObject (0),
      Plugin (Info),
      TimeSlice (Info, TimeSliceTypeSystemTime, TimeSliceModeSingle, 1.0),
      ForgeObserver (Info),
      _state (*(new State (Info))) {

   _init (local);
}


dmz::ForgeQtPluginClient::~ForgeQtPluginClient () {

   delete &_state;
}


// Plugin Interface
void
dmz::ForgeQtPluginClient::update_plugin_state (
      const PluginStateEnum State,
      const UInt32 Level) {

   if (State == PluginStateInit) {

   }
   else if (State == PluginStateStart) {

      // set_time_slice_interval (1.0);
      start_time_slice ();
   }
   else if (State == PluginStateStop) {

   }
   else if (State == PluginStateShutdown) {

   }
}


void
dmz::ForgeQtPluginClient::discover_plugin (
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
dmz::ForgeQtPluginClient::update_time_slice (const Float64 TimeDelta) {

   static Int32 updateCounter (4);
   
   if (_state.forgeModule) {
      
      updateCounter++;
   }

   if (updateCounter == 1) {
      
   }
   else if (updateCounter == 2) {
      
      _state.assetId = _state.forgeModule->create_asset ("me");
      
      _state.forgeModule->store_name (_state.assetId, "scott");
      _state.forgeModule->store_brief (_state.assetId, "shillcock");
      _state.forgeModule->store_details (_state.assetId,"scott shillcock");
      
      StringContainer keywords;
      keywords.append ("male");
      keywords.append ("father");
      keywords.append ("programmer");
      keywords.append ("photographer");
      _state.forgeModule->store_keywords (_state.assetId, keywords);
      
      _state.requestId = _state.forgeModule->put_asset (_state.assetId, this);
      _state.log.warn << "put_asset: " << _state.requestId << endl;
   }
   else if (updateCounter == 3) {
      
      _state.assetId = "me2";
      _state.requestId = _state.forgeModule->get_asset (_state.assetId, this);
      _state.log.warn << "get_asset: " << _state.requestId << endl;
   }
   else if (updateCounter == 4) {
      
      String file ("preview1.jpg");
      _state.requestId = _state.forgeModule->put_asset_preview (_state.assetId, file, this);
      _state.log.warn << "put_preview: " << _state.requestId << endl;
      
      file = "preview2.jpg";
      _state.requestId = _state.forgeModule->put_asset_preview (_state.assetId, file, this);
      _state.log.warn << "put_preview: " << _state.requestId << endl;      

      file = "preview3.png";
      _state.requestId = _state.forgeModule->put_asset_preview (_state.assetId, file, this);
      _state.log.warn << "put_preview: " << _state.requestId << endl;      
   }
   else if (updateCounter == 5) {
      
      _state.assetId = "me5";
      _state.requestId = _state.forgeModule->get_asset (_state.assetId, this);
      _state.log.warn << "get_asset: " << _state.requestId << endl;
   }
   else if (updateCounter == 6) {
      
      _state.assetId = "me123";
      _state.requestId = _state.forgeModule->delete_asset (_state.assetId, this);
      _state.log.warn << "delete_asset: " << _state.requestId << endl;
   }
}


void
dmz::ForgeQtPluginClient::handle_reply (
      const UInt64 RequestId,
      const ForgeTypeEnum &ReqeustType,
      const Boolean Error,
      const StringContainer &Results) {

_state.log.warn << "handle_reply:" << RequestId << endl;

   switch (ReqeustType) {
   
      case ForgeSearch: {
         String assetId;
         while (Results.get_next (assetId)) {

            _state.log.info << "getting asset: " << assetId << endl;
            // _state.forgeModule->get_asset (assetId, this);
         }
         break;
      }
      
      case ForgeGetAsset:
_state.log.debug << "ForgeGetAsset" << endl;

         start_time_slice ();
         break;
         
      case ForgePutAsset:
_state.log.debug << "ForgePutAsset" << endl;

         start_time_slice ();
         break;
      
      case ForgeDeleteAsset:
_state.log.error << "ForgeDeleteAsset" << endl;

         // start_time_slice ();
         break;

      case ForgePutAssetPreview:
_state.log.warn << "ForgePutAssetPreview" << endl;

         if (_state.requestId == RequestId) {
            
            start_time_slice ();
         }
         
         break;

      default:
_state.log.error << "handle_reply: Unknown[" << RequestId << "]" << endl;
   }

   if (Error) {
      
_state.log.error << "Results: " << Results << endl;
   }
   else {
      
_state.log.warn << "Results: " << Results << endl;
   }
}

   
void
dmz::ForgeQtPluginClient::_init (Config &local) {

   _state.forgeModuleName = config_to_string ("module.forge.name", local);
}


extern "C" {

DMZ_PLUGIN_FACTORY_LINK_SYMBOL dmz::Plugin *
create_dmzForgeQtPluginClient (
      const dmz::PluginInfo &Info,
      dmz::Config &local,
      dmz::Config &global) {

   return new dmz::ForgeQtPluginClient (Info, local);
}

};