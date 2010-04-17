#ifndef DMZ_FORGE_MODULE_DOT_H
#define DMZ_FORGE_MODULE_DOT_H

#include <dmzForgeConsts.h>
#include <dmzRuntimePlugin.h>
#include <dmzRuntimeRTTI.h>
#include <dmzTypesBase.h>
#include <dmzTypesString.h>

namespace dmz {
   
   class ForgeObserver;

   class ForgeModule {

      public:
         static ForgeModule *cast (
            const Plugin *PluginPtr,
            const String &PluginName = "");
         
         String get_forge_module_name () const;
         Handle get_forge_module_handle () const;
         
         // ForgeModule Interface
         
         virtual Boolean is_saved (const String &AssetId) = 0;
         
         virtual String create_asset (const String &AssetId = "") = 0;
         
         virtual ForgeAssetTypeEnum lookup_asset_type (const String &AssetId) = 0;
         
         virtual Boolean store_name (const String &AssetId, const String &Value) = 0;
         virtual Boolean lookup_name (const String &AssetId, String &value) = 0;
         
         virtual Boolean store_brief (const String &AssetId, const String &Value) = 0;
         virtual Boolean lookup_brief (const String &AssetId, String &value) = 0;
         
         virtual Boolean store_details (const String &AssetId, const String &Value) = 0;
         virtual Boolean lookup_details (const String &AssetId, String &value) = 0;
         
         virtual Boolean store_keywords (
            const String &AssetId,
            const StringContainer &Value) = 0;
            
         virtual Boolean lookup_keywords (
            const String &AssetId,
            StringContainer &value) = 0;
         
         virtual Boolean lookup_asset_media (
            const String &AssetId,
            StringContainer &value) = 0;
         
         virtual Boolean lookup_previews (
            const String &AssetId,
            StringContainer &value) = 0;
         
         // methods that communicate with api.dmzforge.org
         
         virtual UInt64 search (
            const String &Value,
            ForgeObserver *observer,
            const UInt32 Limit = 0) = 0;
         
         virtual UInt64 get_asset (const String &AssetId, ForgeObserver *observer) = 0;
         virtual UInt64 put_asset (const String &AssetId, ForgeObserver *observer) = 0;
         virtual UInt64 delete_asset (const String &AssetId, ForgeObserver *observer) = 0;
         
         virtual UInt64 get_asset_media (
            const String &AssetId,
            const String &File,
            ForgeObserver *observer) = 0;
         
         virtual UInt64 put_asset_media (
            const String &AssetId,
            const String &File,
            ForgeObserver *observer,
            const String &MimeType = "") = 0;

         virtual UInt64 get_asset_preview (
            const String &AssetId,
            const String &File,
            ForgeObserver *observer) = 0;
         
         virtual UInt64 add_asset_preview (
            const String &AssetId,
            const StringContainer &Files,
            ForgeObserver *observer) = 0;

         virtual UInt64 remove_asset_preview (
            const String &AssetId,
            const StringContainer &Files,
            ForgeObserver *observer) = 0;
         
      protected:
         ForgeModule (const PluginInfo &Info);
         ~ForgeModule ();

      private:
         ForgeModule ();
         ForgeModule (const ForgeModule &);
         ForgeModule &operator= (const ForgeModule &);

         const PluginInfo &__Info;
   };

   //! \cond
   const char ForgeModuleInterfaceName[] = "ForgeModuleInterface";
   //! \endcond
};


inline dmz::ForgeModule *
dmz::ForgeModule::cast (const Plugin *PluginPtr, const String &PluginName) {

   return (ForgeModule *)lookup_rtti_interface (
      ForgeModuleInterfaceName,
      PluginName,
      PluginPtr);
}


inline
dmz::ForgeModule::ForgeModule (const PluginInfo &Info) :
      __Info (Info) {

   store_rtti_interface (ForgeModuleInterfaceName, __Info, (void *)this);
}


inline
dmz::ForgeModule::~ForgeModule () {

   remove_rtti_interface (ForgeModuleInterfaceName, __Info);
}


inline dmz::String
dmz::ForgeModule::get_forge_module_name () const { return __Info.get_name (); }


inline dmz::Handle
dmz::ForgeModule::get_forge_module_handle () const { return __Info.get_handle (); }

#endif // DMZ_FORGE_MODULE_DOT_H
