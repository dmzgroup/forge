#include <dmzForgeConsts.h>
#include <dmzForgeModule.h>
#include "dmzForgePluginSearch.h"
#include <dmzQtConfigRead.h>
#include <dmzRuntimePluginFactoryLinkSymbol.h>
#include <dmzRuntimePluginInfo.h>
#include <dmzTypesStringContainer.h>
#include <QtGui/QtGui>


struct dmz::ForgePluginSearch::ItemStruct {

   const String AssetId;
   UInt64 requestId;
   QListWidgetItem *item;
   QPixmap pixmap;

   ItemStruct (const String &TheAssetId);
};


dmz::ForgePluginSearch::ItemStruct::ItemStruct (const String &TheAssetId) :
      AssetId (TheAssetId),
      requestId (0),
      item (0) {;}


dmz::ForgePluginSearch::ForgePluginSearch (const PluginInfo &Info, Config &local) :
      QFrame (0),
      QtWidget (Info),
      Plugin (Info),
      MessageObserver (Info),
      ForgeObserver (Info),
      _log (Info),
      _convert (Info),
      _forgeModule (0) {

   _ui.setupUi (this);

   _init (local);
}


dmz::ForgePluginSearch::~ForgePluginSearch () {

   _itemTable.clear ();
   _previewTable.clear ();
   _ui.itemListWidget->clear ();
}


// QtWidget Interface
QWidget *
dmz::ForgePluginSearch::get_qt_widget () {

   return this;
}


// Plugin Interface
void
dmz::ForgePluginSearch::update_plugin_state (
      const PluginStateEnum State,
      const UInt32 Level) {

   if (State == PluginStateInit) {

//      show ();
      on_itemListWidget_currentItemChanged (0, 0);
   }
   else if (State == PluginStateStart) {

   }
   else if (State == PluginStateStop) {

   }
   else if (State == PluginStateShutdown) {

   }
}


void
dmz::ForgePluginSearch::discover_plugin (
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
dmz::ForgePluginSearch::receive_message (
      const Message &Type,
      const UInt32 MessageSendHandle,
      const Handle TargetObserverHandle,
      const Data *InData,
      Data *outData) {

}


// ForgeObserver Interface
void
dmz::ForgePluginSearch::handle_reply (
      const UInt64 RequestId,
      const Int32 ReqeustType,
      const Boolean Error,
      const StringContainer &Results) {

   if (Error) {

      _log.error << "Results: " << Results << endl;
   }
   else {

      switch (ReqeustType) {

         case ForgeTypeSearch:
            _handle_search (Results);
            break;

         case ForgeTypeGetAssetPreview: {
            String image;
            Results.get_first (image);
            _handle_get_preview (RequestId, image);
            break;
         }

         default:
            break;
      }
   }
}


void
dmz::ForgePluginSearch::on_searchButton_clicked () {

   if (_forgeModule) {

      const QString SearchText (_ui.searchLineEdit->text ());

      if (!SearchText.isEmpty ()) {

         _itemTable.clear ();
         _ui.itemListWidget->clear ();

         UInt64 requestId = _forgeModule->search (qPrintable (SearchText), this);
      }
   }
}


void
dmz::ForgePluginSearch::on_itemListWidget_currentItemChanged (
      QListWidgetItem *current,
      QListWidgetItem *previous) {

   if (current) {

      const String AssetId (qPrintable (current->data (Qt::UserRole + 1).toString ()));
      ItemStruct *is = _itemTable.lookup (AssetId);
      if (is && _forgeModule) {

         String data;
         _forgeModule->lookup_name (is->AssetId, data);
         _ui.nameLabel->setText (data.get_buffer ());

         _ui.previewLabel->setPixmap (is->pixmap);
      }
   }
   else {

      _ui.nameLabel->clear ();
      _ui.previewLabel->clear ();
   }
}


void
dmz::ForgePluginSearch::on_itemListWidget_itemActivated (QListWidgetItem * item) {

   if (item) {

      const String AssetId (qPrintable (item->data (Qt::UserRole + 1).toString ()));

      Data out = _convert.to_data (AssetId);

      _loadAssetMsg.send (&out);
   }
}


void
dmz::ForgePluginSearch::_handle_search (const StringContainer &Results) {

   StringContainerIterator it;
   String assetId;
   while (Results.get_next (it, assetId)) {

      String name;
      _forgeModule->lookup_name (assetId, name);

      ItemStruct *is = new ItemStruct (assetId);
      is->item = new QListWidgetItem (name.get_buffer (), _ui.itemListWidget);
      is->item->setData (Qt::UserRole + 1, QVariant (is->AssetId.get_buffer ()));

      StringContainer previews;
      _forgeModule->lookup_previews (assetId, previews);

      String image;
      previews.get_first (image);

      is->requestId = _forgeModule->get_asset_preview (assetId, image, this);

      _itemTable.store (is->AssetId, is);
      _previewTable.store (is->requestId, is);
   }

   _ui.searchLineEdit->selectAll ();
}



void
dmz::ForgePluginSearch::_handle_get_preview (
      const UInt64 RequestId,
      const String &Preview) {

   ItemStruct *is = _previewTable.lookup (RequestId);
   if (is) {

      QPixmap pixmap (Preview.get_buffer ());
      is->pixmap = pixmap.scaled (200, 200, Qt::KeepAspectRatio, Qt::SmoothTransformation);
   }
}


void
dmz::ForgePluginSearch::_init (Config &local) {

   RuntimeContext *context (get_plugin_runtime_context ());

   setObjectName (get_plugin_name ().get_buffer ());

   qframe_config_read ("frame", local, this);

   _forgeModuleName = config_to_string ("module.forge.name", local);

   _loadAssetMsg = config_create_message (
      "load-asset-message.name",
      local,
      "Load_Asset_Message",
      context);
}


extern "C" {

DMZ_PLUGIN_FACTORY_LINK_SYMBOL dmz::Plugin *
create_dmzForgePluginSearch (
      const dmz::PluginInfo &Info,
      dmz::Config &local,
      dmz::Config &global) {

   return new dmz::ForgePluginSearch (Info, local);
}

};
