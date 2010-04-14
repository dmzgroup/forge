#ifndef DMZ_FORGE_MODULE_QT_DOT_H
#define DMZ_FORGE_MODULE_QT_DOT_H

#include <dmzForgeConsts.h>
#include <dmzForgeModule.h>
#include <dmzRuntimePlugin.h>
#include <QtCore/QObject>


class QNetworkReply;
class QNetworkRequest;


namespace dmz {

   class ForgeModuleQt :
         public QObject,
         public Plugin,
         protected ForgeModule {
         
      Q_OBJECT
         
      public:
         ForgeModuleQt (const PluginInfo &Info, Config &local);
         ~ForgeModuleQt ();
         
         // Plugin Interface
         virtual void update_plugin_state (
            const PluginStateEnum State,
            const UInt32 Level);
         
         virtual void discover_plugin (
            const PluginDiscoverEnum Mode,
            const Plugin *PluginPtr);
            
         // ForgeModule Interface
         virtual Boolean is_saved (const String &AssetId);
         
         virtual String create_asset (const String &AssetId);
         
         virtual ForgeAssetTypeEnum lookup_asset_type (const String &AssetId);
         
         virtual Boolean store_name (const String &AssetId, const String &Value);
         virtual Boolean lookup_name (const String &AssetId, String &value);
         
         virtual Boolean store_brief (const String &AssetId, const String &Value);
         virtual Boolean lookup_brief (const String &AssetId, String &value);
         
         virtual Boolean store_details (const String &AssetId, const String &Value);
         virtual Boolean lookup_details (const String &AssetId, String &value);
         
         virtual Boolean store_keywords (
            const String &AssetId,
            const StringContainer &Value);
            
         virtual Boolean lookup_keywords (
            const String &AssetId,
            StringContainer &value);
         
         virtual Boolean lookup_asset_media (
            const String &AssetId,
            StringContainer &value);
         
         virtual Boolean lookup_previews (
            const String &AssetId,
            StringContainer &value);
         
         // methods that communicate with api.dmzforge.org
         
         virtual UInt64 search (
            const String &Value,
            ForgeObserver *observer,
            const UInt32 Limit);
            
         virtual UInt64 get_asset (const String &AssetId, ForgeObserver *observer);
         virtual UInt64 put_asset (const String &AssetId, ForgeObserver *observer);
         virtual UInt64 delete_asset (const String &AssetId, ForgeObserver *observer);
         
         virtual UInt64 get_asset_media (
            const String &AssetId,
            const String &File,
            ForgeObserver *observer);
         
         virtual UInt64 put_asset_media (
            const String &AssetId,
            const String &File,
            ForgeObserver *observer,
            const String &MimeType);

         virtual UInt64 get_asset_preview (
            const String &AssetId,
            const String &File,
            ForgeObserver *observer);
         
         virtual UInt64 put_asset_preview (
            const String &AssetId,
            const String &Preview,
            ForgeObserver *observer);

      protected Q_SLOTS:
         void _handle_reply (QNetworkReply *reply);
         void _start_next_upload ();

      protected:
         void _handle_search (const UInt64 RequestId, const String &JsonData);
         void _handle_get_asset (const UInt64 RequestId, const String &JsonData);
         void _handle_put_asset (const UInt64 RequestId, const String &JsonData);
         // void _handle_delete_asset (const UInt64 RequestId, const String &JsonData);
         // void _handle_get_asset_media (const UInt64 RequestId, const String &JsonData);
         // void _handle_put_asset_media (const UInt64 RequestId, const String &JsonData);
         // void _handle_get_asset_preview (const UInt64 RequestId, const String &JsonData);
         // void _handle_put_asset_preview (const UInt64 RequestId, const String &JsonData);
         void _handle_get_uuids (const UInt64 RequestId, const String &JsonData);
         
         void _handle_reply (
            const UInt64 RequestId,
            const ForgeTypeEnum &RequestType,
            StringContainer &Container);
         
         void _handle_error (
            const UInt64 RequestId,
            const ForgeTypeEnum &RequestType,
            const String &Message);
         
         Boolean _store_revision (const String &AssetId, const String &Value);
         Boolean _lookup_revision (const String &AssetId, String &value);
         
         Boolean _asset_to_json (const String &AssetId, String &jsonData);
         Boolean _asset_to_config (const String &AssetId, Config &assetConfig);
         Boolean _config_to_asset (const String &AssetId, const Config &AssetConfig);
         
         String _get_uuid ();
         
         void _init (Config &local);

      protected:
         struct AssetStruct;
         // sgruct UploadStruct;

         struct State;
         State &_state;

      private:
         ForgeModuleQt ();
         ForgeModuleQt (const ForgeModuleQt &);
         ForgeModuleQt &operator= (const ForgeModuleQt &);
   };
};

#endif // DMZ_FORGE_MODULE_QT_DOT_H
