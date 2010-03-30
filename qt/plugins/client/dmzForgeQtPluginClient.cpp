#include <dmzForgeModule.h>
#include "dmzForgeQtPluginClient.h"
#include <dmzRuntimeConfig.h>
#include <dmzRuntimeConfigToTypesBase.h>
#include <dmzRuntimeLog.h>
#include <dmzRuntimePluginFactoryLinkSymbol.h>
#include <dmzRuntimePluginInfo.h>


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

      if (0 && _state.forgeModule) {
         
         UInt64 request = _state.forgeModule->search ("blue", this);
         _state.log.warn << "search request id: " << request << endl;
         
         request = _state.forgeModule->get_asset (
            "6928c289b75540399ac03880a65a96d5", this);
            
         _state.log.warn << "get_asset request id: " << request << endl;

         request = _state.forgeModule->get_asset (
            "884c8da1f7a34502ba6581e63fd823d3", this);
            
         _state.log.warn << "get_asset request id: " << request << endl;

         request = _state.forgeModule->get_asset (
            "35d09081c13747d0b29048e6b79c13f9", this);
            
         _state.log.warn << "get_asset request id: " << request << endl;

         request = _state.forgeModule->get_asset (
            "04178f3be17d4b0d923370373d32a240", this);
            
         _state.log.warn << "get_asset request id: " << request << endl;


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
      const ForgeRequestTypeEnum ReqeustType,
      const Boolean Error,
      const StringContainer &Results) {

   _state.log.warn << "handle_reply: " << RequestId << " - " << (Int32)ReqeustType << endl;
   _state.log.warn << Results << endl;
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
