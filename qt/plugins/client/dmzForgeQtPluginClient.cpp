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
   
   State (const PluginInfo &Info);
};


dmz::ForgeQtPluginClient::State::State (const PluginInfo &Info) :
      log (Info),
      forgeModule (0),
      forgeModuleName () {;}


dmz::ForgeQtPluginClient::ForgeQtPluginClient (const PluginInfo &Info, Config &local) :
      QObject (0),
      Plugin (Info),
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

      if (_state.forgeModule) {
         
         // UInt64 request = _state.forgeModule->search ("blue", this, 50);
         // _state.log.warn << "search request id: " << request << endl;
         //
         // request = _state.forgeModule->get_asset (
         //    "04178f3be17d4b0d923370373d32a240", this);
         //
         // _state.log.warn << "get_asset request id: " << request << endl;
         
         String assetId = _state.forgeModule->create_asset ("1111");
         
         _state.forgeModule->store_name (assetId, "BRDM-2");
         _state.forgeModule->store_brief (assetId, "An amphibious armoured patrol car.");
         
         _state.forgeModule->store_details (assetId,
            "The BRDM-2 (Boyevaya Razvedyvatelnaya Dozornaya Mashina, literally "
            "\"Combat Reconnaissance/Patrol Vehicle\") is an amphibious armoured "
            "patrol car used by Russia and the former Soviet Union. It was also "
            "known under designations BTR-40PB, BTR-40P-2 and GAZ 41-08. This "
            "vehicle, like many other Soviet designs, has been exported extensively "
            "and is in use in at least 38 countries. It was intended to replace the "
            "earlier BRDM-1 with a vehicle that had improved amphibious capabilities "
            "and better armament.");
         
         StringContainer keywords;
         keywords.append ("amphibious");
         keywords.append ("car");
         keywords.append ("russian");
         keywords.append ("damaged");
         _state.forgeModule->store_keywords (assetId, keywords);
         
         UInt64 request = _state.forgeModule->put_asset (assetId, this);
         _state.log.warn << "put asset request id: " << request << endl;
      }
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
dmz::ForgeQtPluginClient::handle_reply (
      const UInt64 RequestId,
      const String &ReqeustType,
      const Boolean Error,
      const StringContainer &Results) {

   if (ReqeustType == ForgeSearchName) {

      String assetId;
      while (Results.get_next (assetId)) {

// _state.log.info << "getting asset: " << assetId << endl;
//         _state.forgeModule->get_asset (assetId, this);
      }
   }
   else if (ReqeustType == ForgePutAssetName) {
      
_state.log.warn << "handle_reply: " << ReqeustType << "[" << RequestId << "]" << endl;
_state.log.warn << Results << endl;
   }
   else {
      
_state.log.error << "handle_reply: " << ReqeustType << "[" << RequestId << "]" << endl;
_state.log.error << Results << endl;
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
