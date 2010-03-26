#include <dmzForgeQtWebService.h>
#include "dmzForgeQtPluginClient.h"
//#include <dmzObjectAttributeMasks.h>
#include <dmzObjectModule.h>
#include <dmzRuntimeConfig.h>
#include <dmzRuntimeConfigToTypesBase.h>
#include <dmzRuntimeConfigToNamedHandle.h>
#include <dmzRuntimeDefinitions.h>
#include <dmzRuntimeObjectType.h>
#include <dmzRuntimeLog.h>
#include <dmzRuntimePluginFactoryLinkSymbol.h>
#include <dmzRuntimePluginInfo.h>
#include <dmzTypesUUID.h>

namespace {

   const char ForgeWebServiceApi[] = "api.dmzforge.org";
   const char ForgeAssetRevisionAttributeName[] = "Forge_Asset_Revision";
   const char ForgeAssetNameAttributeName[] = "Forge_Asset_Name";
   const char ForgeAssetBriefAttributeName[] = "Forge_Asset_Brief";
   const char ForgeAssetDetailsAttributeName[] = "Forge_Asset_Details";
};


struct dmz::ForgeQtPluginClient::State {

   Log log;
   Definitions defs;
   ForgeQtWebService ws;
   ObjectModule *objectModule;
   String objectModuleName;
   Handle revAttrHandle;
   Handle nameAttrHandle;
   Handle briefAttrHandle;
   Handle detailsAttrHandle;
   
   State (const PluginInfo &Info);
};


dmz::ForgeQtPluginClient::State::State (const PluginInfo &Info) :
      log (Info),
      defs (Info, &log),
      ws (0, &log),
      objectModule (0),
      objectModuleName () {;}


dmz::ForgeQtPluginClient::ForgeQtPluginClient (const PluginInfo &Info, Config &local) :
      QObject (0),
      Plugin (Info),
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

      _state.ws.get_asset_list (20);
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

      if (!_state.objectModule) {

         _state.objectModule = ObjectModule::cast (PluginPtr, _state.objectModuleName);
      }
   }
   else if (Mode == PluginDiscoverRemove) {

      if (_state.objectModule && (_state.objectModule == ObjectModule::cast (PluginPtr))) {

         _state.objectModule = 0;
      }
   }
}


void
dmz::ForgeQtPluginClient::_handle_asset_list (const StringContainer &Results) {

   String id;
   
   while (Results.get_next (id)) {
     
      _state.ws.get_asset (id);
   };
}


void
dmz::ForgeQtPluginClient::_handle_asset (const Config &Results) {

   if (_state.objectModule) {
      
      Boolean activated (True);
      
      const String Id (config_to_string ("_id", Results));
      
      ObjectType assetType;
      _state.defs.lookup_object_type ("asset", assetType);
      
      Handle assetHandle = _state.objectModule->lookup_handle_from_uuid (Id);
      if (!assetHandle) {
         
         assetHandle = _state.objectModule->create_object (assetType, ObjectLocal);
         activated = False;
      }

      if (assetHandle && Id) {

         const String Rev (config_to_string ("_rev", Results));
         const String Name (config_to_string ("name", Results));
         const String Brief (config_to_string ("brief", Results));
         const String Details (config_to_string ("name", Results));

         _state.objectModule->store_text (assetHandle, _state.revAttrHandle, Rev);
         _state.objectModule->store_text (assetHandle, _state.nameAttrHandle, Name);
         _state.objectModule->store_text (assetHandle, _state.briefAttrHandle, Brief);
         _state.objectModule->store_text (assetHandle, _state.detailsAttrHandle, Details);

         if (!activated) {
            
            _state.objectModule->store_uuid (assetHandle, Id);
            _state.objectModule->activate_object (assetHandle);
         }
         
         // _state.assetTable.store (assetHandle, Id);
      }
   }
}


void
dmz::ForgeQtPluginClient::_init (Config &local) {

   _state.objectModuleName = config_to_string ("module.object.name", local);

   _state.revAttrHandle = _state.defs.lookup_named_handle (ForgeAssetRevisionAttributeName);
   _state.nameAttrHandle = _state.defs.lookup_named_handle (ForgeAssetNameAttributeName);
   _state.briefAttrHandle = _state.defs.lookup_named_handle (ForgeAssetBriefAttributeName);
   _state.detailsAttrHandle = _state.defs.lookup_named_handle (ForgeAssetDetailsAttributeName);

   String host = config_to_string ("webservice.host", local, ForgeWebServiceApi);
   Int32 port = config_to_int32 ("webservice.port", local, 80);
   
   _state.ws.set_host (host);
   _state.ws.set_port (port);
   
   connect (
      &_state.ws,
      SIGNAL (asset_list_fetched (const StringContainer &)),
      SLOT (_handle_asset_list (const StringContainer &)));
      
   connect (
      &_state.ws,
      SIGNAL (asset_fetched (const Config &)),
      SLOT (_handle_asset (const Config &)));
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
