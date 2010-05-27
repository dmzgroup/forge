#include <dmzForgeConsts.h>
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

   static Int32 updateCounter (-1);
   
   if (_state.forgeModule) {
      
      updateCounter++;
   }

   _state.assetId = "aaaaaa";
   
   if (updateCounter == 0) {
      
      _state.requestId = _state.forgeModule->get_asset (_state.assetId, this);
_state.log.debug << " --> get_asset: " << _state.requestId << endl;
updateCounter = 6;
   }
   else if (updateCounter == 1) {
      
//       _state.requestId = _state.forgeModule->delete_asset (_state.assetId, this);
// _state.log.debug << " --> delete_asset: " << _state.requestId << endl;
   }
   else if (updateCounter == 2) {
      
//       _state.assetId = _state.forgeModule->create_asset (_state.assetId);
//       
//       _state.forgeModule->store_name (_state.assetId, "the name");
//       _state.forgeModule->store_brief (_state.assetId, "the brief");
//       _state.forgeModule->store_details (_state.assetId,"details details details");
//       
//       StringContainer keywords;
//       keywords.append ("key1");
//       keywords.append ("key2");
//       keywords.append ("key3");
//       keywords.append ("key4");
//       keywords.append ("key5");
//       keywords.append ("key6");
//       _state.forgeModule->store_keywords (_state.assetId, keywords);
//       
//       _state.requestId = _state.forgeModule->put_asset (_state.assetId, this);
// _state.log.debug << "--> put_asset: " << _state.requestId << endl;
// updateCounter++;
   }
   else if (updateCounter == 3) {
      
//       _state.requestId = _state.forgeModule->get_asset (_state.assetId, this);
// _state.log.warn << updateCounter << ":get_asset: " << _state.requestId << endl;
//       start_time_slice ();
   }
   else if (updateCounter == 4) {
      
//       StringContainer previews;
//       previews.append ("preview1.jpg");
//       previews.append ("preview2.jpg");
//       previews.append ("preview3.jpg");
//       
//       _state.requestId = _state.forgeModule->add_asset_preview (_state.assetId, previews, this);
// _state.log.debug << "--> add_asset_preview: " << _state.requestId << endl;
   }
   else if (updateCounter == 5) {
      
//       String file ("preview0.jpg");
//       
//      _state.requestId = _state.forgeModule->put_asset_media (_state.assetId, file, this);
// _state.log.debug << "--> put_asset_media: " << _state.requestId << endl;
   }
   else if (updateCounter == 6) {

//       String file ("1-1d7046f4b0414d1da2bdc624f3d85a86cb114265.jpg");
//       
//       _state.requestId = _state.forgeModule->get_asset_media (_state.assetId, file, this);
// _state.log.debug << "--> get_asset_media: " << _state.requestId << endl;
   }
   else if (updateCounter == 7) {
      
      String file ("preview1.jpg");

      _state.requestId = _state.forgeModule->get_asset_preview (_state.assetId, file, this);
_state.log.debug << "--> get_asset_preview: " << _state.requestId << endl;
   }
   else if (updateCounter == 8) {
      
      String file ("preview2.jpg");

      _state.requestId = _state.forgeModule->get_asset_preview (_state.assetId, file, this);
_state.log.debug << "--> get_asset_preview: " << _state.requestId << endl;
   }
   else if (updateCounter == 9) {
      
      String file ("preview1.jpg");

      _state.requestId = _state.forgeModule->get_asset_preview (_state.assetId, file, this);
_state.log.debug << "--> get_asset_preview: " << _state.requestId << endl;
   }
   else if (updateCounter == 10) {

      String file ("preview2.jpg");

      _state.requestId = _state.forgeModule->get_asset_preview (_state.assetId, file, this);
_state.log.debug << "--> get_asset_preview: " << _state.requestId << endl;
   }
}


void
dmz::ForgeQtPluginClient::handle_reply (
      const UInt64 RequestId,
      const Int32 ReqeustType,
      const Boolean Error,
      const StringContainer &Results) {

// _state.log.warn << "handle_reply:" << RequestId << endl;

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
_state.log.error << "<-- ForgeGetAsset" << endl;

         start_time_slice ();
         break;
         
      case ForgeTypePutAsset:
_state.log.error << "<-- ForgePutAsset" << endl;

         start_time_slice ();
         break;
      
      case ForgeTypeDeleteAsset:
_state.log.error << "<-- ForgeDeleteAsset" << endl;

         start_time_slice ();
         break;

      case ForgeTypeGetAssetMedia:
_state.log.error << "<-- ForgeTypeGetAssetMedia" << endl;

         start_time_slice ();
         break;

      case ForgeTypePutAssetMedia:
_state.log.error << "<-- ForgePutAssetMedia" << endl;

         start_time_slice ();
         break;

      case ForgeTypeAddAssetPreview:
_state.log.error << "<-- ForgeAddAssetPreview" << endl;

         start_time_slice ();
         break;

      default:
_state.log.error << "handle_reply: Unknown[" << RequestId << "]" << endl;
   }

   if (Error) {
      
_state.log.error << "Results: " << Results << endl;
   }
   else {
      
// _state.log.warn << "Results: " << Results << endl;
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
