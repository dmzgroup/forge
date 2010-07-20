#ifndef DMZ_FORGE_PLUGIN_SEARCH_DOT_H
#define DMZ_FORGE_PLUGIN_SEARCH_DOT_H

#include <dmzRuntimeLog.h>
#include <dmzForgeObserver.h>
#include <dmzQtWidget.h>
#include <dmzRuntimeDataConverterTypesBase.h>
#include <dmzRuntimeMessaging.h>
#include <dmzRuntimePlugin.h>
#include <dmzTypesHashTableUInt64Template.h>
#include <QtGui/QFrame>
#include <QtGui/QStandardItemModel>
#include "ui_SearchForm.h"

class QListWidgetItem;


namespace dmz {

   class ForgeModule;

   class ForgePluginSearch :
         public QFrame,
         public QtWidget,
         public Plugin,
         public MessageObserver,
         public ForgeObserver {

      Q_OBJECT

      public:
         ForgePluginSearch (const PluginInfo &Info, Config &local);
         ~ForgePluginSearch ();

         // QtWidget Interface
         virtual QWidget *get_qt_widget ();

         // Plugin Interface
         virtual void update_plugin_state (
            const PluginStateEnum State,
            const UInt32 Level);

         virtual void discover_plugin (
            const PluginDiscoverEnum Mode,
            const Plugin *PluginPtr);

         // Message Observer Interface
         virtual void receive_message (
            const Message &Type,
            const UInt32 MessageSendHandle,
            const Handle TargetObserverHandle,
            const Data *InData,
            Data *outData);

         // ForgeObserver Interface
         virtual void handle_reply (
            const UInt64 RequestId,
            const Int32 ReqeustType,
            const Boolean Error,
            const StringContainer &Results);

      protected Q_SLOTS:
         void on_searchButton_clicked ();

         void on_listView_activated (const QModelIndex &);

      protected:
         void _handle_search (const StringContainer &Results);
         void _handle_get_preview (const UInt64 RequestId, const String &Preview);

         void _init (Config &local);

         struct ItemStruct;

         Log _log;
         DataConverterString _convert;
         Ui::SearchForm _ui;
         ForgeModule *_forgeModule;
         String _forgeModuleName;
         Message _cleanupMsg;
         Message _loadAssetMsg;
         QStandardItemModel _model;
         HashTableUInt64Template<QStandardItem> _itemTable;

      private:
         ForgePluginSearch ();
         ForgePluginSearch (const ForgePluginSearch &);
         ForgePluginSearch &operator= (const ForgePluginSearch &);
   };
};


#endif // DMZ_FORGE_PLUGIN_SEARCH_DOT_H
