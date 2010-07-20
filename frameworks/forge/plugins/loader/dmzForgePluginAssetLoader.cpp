#include "dmzForgePluginAssetLoader.h"
#include <dmzForgeModule.h>
#include <dmzObjectAttributeMasks.h>
#include <dmzObjectModule.h>
#include <dmzRuntimeConfig.h>
#include <dmzRuntimeConfigToNamedHandle.h>
#include <dmzRuntimeConfigToTypesBase.h>
#include <dmzRuntimePluginFactoryLinkSymbol.h>
#include <dmzRuntimePluginInfo.h>
#include <dmzTypesStringContainer.h>
#include <dmzTypesVector.h>


dmz::ForgePluginAssetLoader::ForgePluginAssetLoader (
      const PluginInfo &Info,
      Config &local) :
      Plugin (Info),
      ForgeObserver (Info),
      MessageObserver (Info),
      ObjectObserverUtil (Info, local),
      _log (Info),
      _convert (Info),
      _forgeModule (0),
      _objectHandle (0),
      _defaultAttrHandle (0),
      _modelAttrHandle(0),
      _requestId (0) {

   _init (local);
}


dmz::ForgePluginAssetLoader::~ForgePluginAssetLoader () {

}


// Plugin Interface
void
dmz::ForgePluginAssetLoader::update_plugin_state (
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
dmz::ForgePluginAssetLoader::discover_plugin (
      const PluginDiscoverEnum Mode,
      const Plugin *PluginPtr) {

   if (Mode == PluginDiscoverAdd) {

      if (!_forgeModule) {

         _forgeModule = ForgeModule::cast (PluginPtr, _forgeModuleName);
      }
   }
   else if (Mode == PluginDiscoverRemove) {

      if (_forgeModule && (_forgeModule == ForgeModule::cast (PluginPtr))) {

         _forgeModule = 0;
      }
   }
}


// Forge Observer Interface
void
dmz::ForgePluginAssetLoader::handle_reply (
      const UInt64 RequestId,
      const Int32 ReqeustType,
      const Boolean Error,
      const dmz::StringContainer &Results) {

   switch (ReqeustType) {

      case ForgeTypeSearch:
         break;

      case ForgeTypeGetAsset:
         break;

      case ForgeTypePutAsset:
         break;

      case ForgeTypeDeleteAsset:
         break;

      case ForgeTypeGetAssetMedia: {

_log.warn << "ForgeTypeGetAssetMedia: " << Results << endl;

         ObjectModule *objMod = get_object_module ();

         if (objMod) {

            _objectHandle = objMod->create_object (_type, ObjectLocal);

            String modelFile;
            Results.get_first (modelFile);

            objMod->store_text (_objectHandle, _modelAttrHandle, modelFile);

            objMod->store_position (_objectHandle, _defaultAttrHandle, Vector ());

            objMod->activate_object (_objectHandle);
         }

         break;
      }

      case ForgeTypePutAssetMedia:
         break;

      case ForgeTypeAddAssetPreview:
         break;

      default:
         break;
   }
}


// Message Observer Interface
void
dmz::ForgePluginAssetLoader::receive_message (
      const Message &Type,
      const UInt32 MessageSendHandle,
      const Handle TargetObserverHandle,
      const Data *InData,
      Data *outData) {

   if (Type == _loadAssetMsg) {

      if (_objectHandle) {

      }

      _assetId = _convert.to_string (InData);

      if (_assetId && _forgeModule) {

         StringContainer container;
         if (_forgeModule->lookup_asset_media (_assetId, container)) {

            String file;
            container.get_first (file);

            _requestId = _forgeModule->get_asset_media (_assetId, file, this);
         }
      }
   }
}


// Object Observer Interface
void
dmz::ForgePluginAssetLoader::destroy_object (
      const UUID &Identity,
      const Handle ObjectHandle) {

}


// ForgePluginAssetLoader Interface
void
dmz::ForgePluginAssetLoader::_init (Config &local) {

   RuntimeContext *context (get_plugin_runtime_context ());

   _defaultAttrHandle = activate_default_object_attribute (ObjectDestroyMask);

   _loadAssetMsg = config_create_message (
      "load-asset-message.name",
      local,
      "Load_Asset_Message",
      context);

   subscribe_to_message (_loadAssetMsg);

   const String TypeName (config_to_string ("object-type.name", local));
   _type.set_type (TypeName, context);

   _modelAttrHandle = config_to_named_handle (
      "attribute-model.name", local, "Object_Model_Attribute", context);
}


extern "C" {

DMZ_PLUGIN_FACTORY_LINK_SYMBOL dmz::Plugin *
create_dmzForgePluginAssetLoader (
      const dmz::PluginInfo &Info,
      dmz::Config &local,
      dmz::Config &global) {

   return new dmz::ForgePluginAssetLoader (Info, local);
}

};
