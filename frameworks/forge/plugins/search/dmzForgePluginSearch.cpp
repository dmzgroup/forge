#include <dmzForgeConsts.h>
#include <dmzForgeModule.h>
#include "dmzForgePluginSearch.h"
#include <dmzQtConfigRead.h>
#include <dmzRuntimePluginFactoryLinkSymbol.h>
#include <dmzRuntimePluginInfo.h>
#include <dmzTypesStringContainer.h>
#include <QtGui/QtGui>


namespace {

   static const dmz::Int32 LocalItemIconWidth (200);
   static const dmz::Int32 LocalItemIconHeight (150);
};


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

   _ui.listView->setModel (&_model);
   _ui.listView->setIconSize (QSize(LocalItemIconWidth, LocalItemIconHeight));
   _ui.listView->setGridSize (QSize(LocalItemIconWidth + 20, LocalItemIconHeight + 25));

   _init (local);
}


dmz::ForgePluginSearch::~ForgePluginSearch () {

   _model.clear ();
   _itemTable.clear ();
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

      _model.clear ();
      _itemTable.clear ();

      if (!SearchText.isEmpty ()) {

         _forgeModule->search (qPrintable (SearchText), this);
      }
   }
}


void
dmz::ForgePluginSearch::on_listView_activated (const QModelIndex &Index) {

   QStandardItem *item = _model.itemFromIndex (Index);
   if (item) {

      _cleanupMsg.send ();

      const String AssetId (qPrintable (item->data ().toString ()));

      Data out = _convert.to_data (AssetId);
      _loadAssetMsg.send (&out);
   }
}


void
dmz::ForgePluginSearch::_handle_search (const StringContainer &Results) {

   QStandardItem *rootItem = _model.invisibleRootItem ();

   if (_forgeModule && rootItem) {

      const QPixmap DefaultPixmap (":/search/default.png");

      const QIcon DefaultIcon (
         DefaultPixmap.scaled (
            LocalItemIconWidth,
            LocalItemIconHeight,
            Qt::IgnoreAspectRatio,
            Qt::SmoothTransformation));

      StringContainerIterator it;
      String assetId;

      while (Results.get_next (it, assetId)) {

         String name;
         _forgeModule->lookup_name (assetId, name);

         QStandardItem *item = new QStandardItem (DefaultIcon, name.get_buffer ());
         item->setData (QVariant (assetId.get_buffer ()));

         rootItem->appendRow (item);

         StringContainer previews;
         _forgeModule->lookup_previews (assetId, previews);

         String image;
         previews.get_first (image);

         if (image) {

            UInt64 requestId = _forgeModule->get_asset_preview (assetId, image, this);
            if (requestId) { _itemTable.store (requestId, item); }
         }
      }
   }

   _ui.searchLineEdit->selectAll ();
}



void
dmz::ForgePluginSearch::_handle_get_preview (
      const UInt64 RequestId,
      const String &Preview) {

   QStandardItem *item = _itemTable.remove (RequestId);
   if (item) {

      const QPixmap PreviewPixmap (Preview.get_buffer ());

      const QIcon PreviewIcon (
         PreviewPixmap.scaled (
            LocalItemIconWidth,
            LocalItemIconHeight,
            Qt::IgnoreAspectRatio,
            Qt::SmoothTransformation));

      item->setIcon (PreviewIcon);
   }
}


void
dmz::ForgePluginSearch::_init (Config &local) {

   RuntimeContext *context (get_plugin_runtime_context ());

   setObjectName (get_plugin_name ().get_buffer ());

   qframe_config_read ("frame", local, this);

   _forgeModuleName = config_to_string ("module.forge.name", local);

   _cleanupMsg = config_create_message (
      "cleanup-message.name",
      local,
      "CleanupObjectsMessage",
      context);

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
