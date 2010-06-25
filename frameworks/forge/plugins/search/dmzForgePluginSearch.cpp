#include <dmzForgeConsts.h>
#include <dmzForgeModule.h>
#include "dmzForgePluginSearch.h"
#include <dmzQtConfigRead.h>
#include <dmzRuntimePluginFactoryLinkSymbol.h>
#include <dmzRuntimePluginInfo.h>
#include <dmzTypesStringContainer.h>
#include <QtGui/QtGui>


dmz::ForgePluginSearch::ForgePluginSearch (const PluginInfo &Info, Config &local) :
      QFrame (0),
      QtWidget (Info),
      Plugin (Info),
      MessageObserver (Info),
      ForgeObserver (Info),
      _log (Info),
      _forgeModule (0),
      _requestId (0) {

   _ui.setupUi (this);

   _init (local);
}


dmz::ForgePluginSearch::~ForgePluginSearch () {

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

      show ();
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

   switch (ReqeustType) {

      case ForgeTypeSearch: {
         StringContainerIterator it;
         String assetId;
         _log.info << "Search results: " << endl;
         while (Results.get_next (it, assetId)) {

             _log.info << "asset: " << assetId << endl;
            // _state.forgeModule->get_asset (assetId, this);
         }
         break;
      }

      default:
         break;
   }

   if (Error) {

_log.error << "Results: " << Results << endl;
   }
}


void
dmz::ForgePluginSearch::on_searchButton_clicked () {

   if (_forgeModule && !_requestId) {

      const QString SearchText (_ui.searchLineEdit->text ());

      if (!SearchText.isEmpty ()) {

         _requestId = _forgeModule->search (qPrintable (SearchText), this);
      }
   }
}


void
dmz::ForgePluginSearch::_init (Config &local) {

   setObjectName (get_plugin_name ().get_buffer ());

   qframe_config_read ("frame", local, this);

   _forgeModuleName = config_to_string ("module.forge.name", local);
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
