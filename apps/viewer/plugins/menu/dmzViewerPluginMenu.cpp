#include <dmzObjectAttributeMasks.h>
#include <dmzObjectModule.h>
#include <dmzQtModuleMainWindow.h>
#include <dmzQtUtil.h>
#include <dmzRuntimeConfigToNamedHandle.h>
#include <dmzRuntimeDefinitions.h>
#include <dmzRuntimePluginFactoryLinkSymbol.h>
#include <dmzRuntimePluginInfo.h>
#include <dmzSystemFile.h>
#include "dmzViewerPluginMenu.h"
#include <QtGui/QtGui>


dmz::ViewerPluginMenu::ViewerPluginMenu (
      const PluginInfo &Info,
      Config &local) :
      QObject (0),
      Plugin (Info),
      ObjectObserverUtil (Info, local),
      _log (Info),
      _appState (Info),
      _mainWindowModule (0),
      _mainWindowModuleName (),
      _menuTable (),
      _type (),
      _objectHandle (0),
      _defaultAttrHandle (0),
      _modelAttrHandle (0) {

   _init (local);
}


dmz::ViewerPluginMenu::~ViewerPluginMenu () {

   _menuTable.empty ();
}


// Plugin Interface
void
dmz::ViewerPluginMenu::update_plugin_state (
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
dmz::ViewerPluginMenu::discover_plugin (
      const PluginDiscoverEnum Mode,
      const Plugin *PluginPtr) {

   if (Mode == PluginDiscoverAdd) {

      if (!_mainWindowModule) {

         _mainWindowModule = QtModuleMainWindow::cast (PluginPtr, _mainWindowModuleName);
         if (_mainWindowModule) {

            HashTableStringIterator it;
            MenuStruct *ms (0);

            while (_menuTable.get_next (it, ms)) {

               foreach (QAction *action, ms->actionList) {

                  _mainWindowModule->add_menu_action (ms->Name, action);
               }
            }
         }
      }
   }
   else if (Mode == PluginDiscoverRemove) {

      if (_mainWindowModule &&
            (_mainWindowModule == QtModuleMainWindow::cast (PluginPtr))) {

         HashTableStringIterator it;
         MenuStruct *ms (0);

         while (_menuTable.get_next (it, ms)) {

            foreach (QAction *action, ms->actionList) {

               _mainWindowModule->remove_menu_action (ms->Name, action);
            }
         }

         _mainWindowModule = 0;
      }
   }
}


// Object Observer Interface
void
dmz::ViewerPluginMenu::destroy_object (
      const UUID &Identity,
      const Handle ObjectHandle) {

   if (ObjectHandle == _objectHandle) {

      _objectHandle = 0;

      QMainWindow *mainWindow = _mainWindowModule->get_qt_main_window ();
      if (mainWindow) {

         QString title (_mainWindowModule->get_window_name ());
         mainWindow->setWindowTitle (title);
      }
   }
}


void
dmz::ViewerPluginMenu::on_openAction_triggered () {

   const QString FileName = QFileDialog::getOpenFileName (
      _mainWindowModule ? _mainWindowModule->get_qt_main_window () : 0,
      tr ("Load File"),
      _get_last_path (),
      QString ("*.ive | *.dae | *.flt"));

   if (!FileName.isEmpty ()) { _open_file (FileName); }
}


void
dmz::ViewerPluginMenu::_open_file (const QString &FileName) {

   ObjectModule *objMod = get_object_module ();

   QFileInfo fi (FileName);
   if (fi.exists () && objMod && _mainWindowModule) {

      if (_objectHandle) { objMod->destroy_object (_objectHandle); }

      _objectHandle = objMod->create_object (_type, ObjectLocal);

      const String ModelFile (qPrintable (fi.absoluteFilePath ()));

      objMod->store_text (_objectHandle, _modelAttrHandle, ModelFile);

      objMod->store_position (_objectHandle, _defaultAttrHandle, Vector ());

      objMod->activate_object (_objectHandle);

      _appState.set_default_directory (ModelFile);

      QMainWindow *mainWindow = _mainWindowModule->get_qt_main_window ();
      if (mainWindow) {

         QString title (_mainWindowModule->get_window_name ());
         mainWindow->setWindowTitle (title + ": " + fi.fileName ());
      }
   }
}


QString
dmz::ViewerPluginMenu::_get_last_path () {

   String lastPath (_appState.get_default_directory ());

   if (!is_valid_path (lastPath)) {

      lastPath.flush () << get_home_directory ();
   }

   QFileInfo fi (lastPath.get_buffer ());

   return fi.path ();
}


void
dmz::ViewerPluginMenu::_init_action_list (Config &actionList, MenuStruct &ms) {

   ConfigIterator it;
   Config actionConfig;

   while (actionList.get_next_config (it, actionConfig)) {

      QAction *action (new QAction (this));
      qaction_config_read ("", actionConfig, action);

#ifdef Q_WS_MAC
      action->setIconVisibleInMenu (False);
#endif

      ms.actionList.append (action);
   }
}


void
dmz::ViewerPluginMenu::_init_menu_list (Config &menuList) {

   ConfigIterator it;
   Config menu;

   while (menuList.get_next_config (it, menu)) {

      const String MenuName (config_to_string ("name", menu));

      MenuStruct *ms = _menuTable.lookup (MenuName);

      if (!ms) {

         ms = new MenuStruct (MenuName);
         if (!_menuTable.store (ms->Name, ms)) { delete ms; ms = 0; }
      }

      if (ms) {

         Config actionList;

         if (menu.lookup_all_config ("action", actionList)) {

            _init_action_list (actionList, *ms);
         }
      }
   }
}


void
dmz::ViewerPluginMenu::_init (Config &local) {

   setObjectName (get_plugin_name ().get_buffer ());

   RuntimeContext *context (get_plugin_runtime_context ());

   _mainWindowModuleName = config_to_string ("module.main-window.name", local);

   Config menuList;
   if (local.lookup_all_config ("menu", menuList)) { _init_menu_list (menuList); }

   QMetaObject::connectSlotsByName (this);

   const String TypeName (config_to_string ("object-type.name", local));
   _type.set_type (TypeName, context);

   _modelAttrHandle = config_to_named_handle (
      "attribute.model.name", local, "Object_Model_Attribute", context);

   _defaultAttrHandle = activate_default_object_attribute (ObjectDestroyMask);
}


extern "C" {

DMZ_PLUGIN_FACTORY_LINK_SYMBOL dmz::Plugin *
create_dmzViewerPluginMenu (
      const dmz::PluginInfo &Info,
      dmz::Config &local,
      dmz::Config &global) {

   return new dmz::ViewerPluginMenu (Info, local);
}

};
