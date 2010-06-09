#ifndef DMZ_ASSET_PUBLISHER_DOT_H
#define DMZ_ASSET_PUBLISHER_DOT_H

#include <dmzForgeObserver.h>
#include <dmzRuntimeLog.h>
#include <dmzRuntimePluginInfo.h>
#include <dmzTypesStringContainer.h>
#include <QtCore/QQueue>
#include <ui_PublishForm.h>


namespace dmz {

   class ForgeModule;

   class AssetPublisher : public Ui::PublishForm, public ForgeObserver {

      public:
         AssetPublisher (const PluginInfo &Info);
         ~AssetPublisher ();

         void setupUi (QWidget *parent);

         void set_forge_module (ForgeModule *forge);

         void add_media (const String &File);

         //void set_asset (const String &AssetId);

         void publish ();
         void cancel ();
         void reset ();

         // ForgeObserver Interface
         virtual void handle_reply (
            const UInt64 RequestId,
            const Int32 ReqeustType,
            const Boolean Error,
            const StringContainer &Results);

      protected:
         void _publish_media ();
         void _publish_previews ();
         void _publish_finished ();
         void _publish_error (const String &Error);
         void _begin ();
         void _end ();

      protected:
         Log _log;
         ForgeModule *_forge;
         String _assetId;
         UInt64 _requestId;
         StringContainer _media;
         StringContainer _previews;
         Boolean _deleteAsset;
   };
};


#endif // DMZ_ASSET_PUBLISHER_DOT_H
