#include "dmzForgePluginAssetLoader.h"
#include <dmzForgeModule.h>
#include <dmzObjectAttributeMasks.h>
#include <dmzObjectModule.h>
#include <dmzRuntimeConfig.h>
#include <dmzRuntimeConfigToTypesBase.h>
#include <dmzRuntimePluginFactoryLinkSymbol.h>
#include <dmzRuntimePluginInfo.h>


dmz::ForgePluginAssetLoader::ForgePluginAssetLoader (
      const PluginInfo &Info,
      Config &local) :
      Plugin (Info),
      MessageObserver (Info),
      ObjectObserverUtil (Info, local),
      _log (Info),
      _convert (Info),
      _forgeModule (0),
      _defaultAttrHandle (0) {

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


// Message Observer Interface
void
dmz::ForgePluginAssetLoader::receive_message (
      const Message &Type,
      const UInt32 MessageSendHandle,
      const Handle TargetObserverHandle,
      const Data *InData,
      Data *outData) {

   if (Type == _loadAssetMsg) {

      ObjectModule *objMod = get_object_module ();

      const String &AssetId (_convert.to_string (InData));

      if (AssetId && objMod) {

//         _objectHandle = objMod->create_object (_type, ObjectLocal);

//         const String ModelFile (qPrintable (fi.absoluteFilePath ()));

//         objMod->store_text (_objectHandle, _modelAttrHandle, ModelFile);

//         objMod->store_position (_objectHandle, _defaultAttrHandle, Vector ());

//         objMod->activate_object (_objectHandle);
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
