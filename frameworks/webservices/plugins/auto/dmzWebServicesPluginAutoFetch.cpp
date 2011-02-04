#include "dmzWebServicesPluginAutoFetch.h"
#include <dmzRuntimePluginFactoryLinkSymbol.h>
#include <dmzRuntimePluginInfo.h>

dmz::WebServicesPluginAutoFetch::WebServicesPluginAutoFetch (const PluginInfo &Info, Config &local) :
      Plugin (Info),
      _log (Info) {

   _init (local);
}


dmz::WebServicesPluginAutoFetch::~WebServicesPluginAutoFetch () {

}


// Plugin Interface
void
dmz::WebServicesPluginAutoFetch::update_plugin_state (
      const PluginStateEnum State,
      const UInt32 Level) {

   if (State == PluginStateInit) {

   }
   else if (State == PluginStateStart) {

   }
   else if (State == PluginStateStop) {

   }
   else if (State == PluginStateShutdown) {

   }
}


void
dmz::WebServicesPluginAutoFetch::discover_plugin (
      const PluginDiscoverEnum Mode,
      const Plugin *PluginPtr) {

   if (Mode == PluginDiscoverAdd) {

   }
   else if (Mode == PluginDiscoverRemove) {

   }
}


// WebServicesPluginAutoFetch Interface
void
dmz::WebServicesPluginAutoFetch::_init (Config &local) {

}


extern "C" {

DMZ_PLUGIN_FACTORY_LINK_SYMBOL dmz::Plugin *
create_dmzWebServicesPluginAutoFetch (
      const dmz::PluginInfo &Info,
      dmz::Config &local,
      dmz::Config &global) {

   return new dmz::WebServicesPluginAutoFetch (Info, local);
}

};
