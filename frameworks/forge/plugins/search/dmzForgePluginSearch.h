#ifndef DMZ_FORGE_PLUGIN_SEARCH_DOT_H
#define DMZ_FORGE_PLUGIN_SEARCH_DOT_H

#include <dmzRuntimeLog.h>
#include <dmzForgeObserver.h>
#include <dmzQtWidget.h>
#include <dmzRuntimeMessaging.h>
#include <dmzRuntimePlugin.h>
#include <QtGui/QFrame>
#include <ui_SearchForm.h>


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

      protected:
         void _init (Config &local);

         Log _log;
         Ui::SearchForm _ui;
         ForgeModule *_forgeModule;
         String _forgeModuleName;
         UInt64 _requestId;


      private:
         ForgePluginSearch ();
         ForgePluginSearch (const ForgePluginSearch &);
         ForgePluginSearch &operator= (const ForgePluginSearch &);
   };
};


#endif // DMZ_FORGE_PLUGIN_SEARCH_DOT_H
