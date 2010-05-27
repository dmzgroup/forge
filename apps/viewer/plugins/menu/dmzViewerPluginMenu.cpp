#include <dmzInputEventMasks.h>
#include <dmzInputEventMouse.h>
#include <dmzQtModuleMainWindow.h>
#include <dmzQtUtil.h>
#include <dmzRenderConsts.h>
#include <dmzRenderModuleCoreOSG.h>
#include <dmzRenderModulePortal.h>
#include <dmzRuntimePluginFactoryLinkSymbol.h>
#include <dmzRuntimePluginInfo.h>
#include <dmzSystemFile.h>
#include <dmzTypesMatrix.h>
#include <dmzTypesConsts.h>
#include "dmzViewerPluginMenu.h"
#include <osgDB/ReadFile>
#include <QtGui/QtGui>


dmz::ViewerPluginMenu::ViewerPluginMenu (
      const PluginInfo &Info,
      Config &local) :
      QObject (0),
      Plugin (Info),
      InputObserverUtil (Info, local),
      _log (Info),
      _appState (Info),
      _mainWindowModule (0),
      _mainWindowModuleName (),
      _core (0),
      _portal (0),
      _menuTable (),
      _node (),
      _nodeFile (),
      _radius (0.0),
      _heading (0.0),
      _pitch (0.0) {

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

      if (!_core) { _core = RenderModuleCoreOSG::cast (PluginPtr); }
      if (!_portal) { _portal = RenderModulePortal::cast (PluginPtr); }
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

      if (_core && (_core == RenderModuleCoreOSG::cast (PluginPtr))) { _core = 0; }
      if (_portal && (_portal == RenderModulePortal::cast (PluginPtr))) { _portal = 0; }
   }
}


// Input Observer Util Interface
void
dmz::ViewerPluginMenu::receive_mouse_event (
      const Handle Channel,
      const InputEventMouse &Value) {

   if (_portal) {

      Boolean updatePortal = False;

      const Float64 SizeY = Float64 (Value.get_window_size_y ());

      if (Value.is_button_pressed (1)) {

         const Float64 SizeX = Float64 (Value.get_window_size_x ());

         const Float64 PercentX =
            Float64 (Value.get_mouse_delta_x ()) / (SizeX > 0.0 ? SizeX : 0.0);

         const Float64 PercentY =
            Float64 (Value.get_mouse_delta_y ()) / (SizeY > 0.0 ? SizeY : 0.0);

         _heading -= PercentX * Pi64;
         _pitch -= PercentY * Pi64;

         if (_heading < 0) { _heading += TwoPi64; }
         else if (_heading > TwoPi64) { _heading -= TwoPi64; }

         if (_pitch < 0) { _pitch += TwoPi64; }
         else if (_pitch > TwoPi64) { _pitch -= TwoPi64; }

         updatePortal = True;
      }

      if (Value.is_button_pressed (3)) {

         const Float64 PercentY =
            Float64 (Value.get_mouse_delta_y ()) / (SizeY > 0.0 ? SizeY : 0.0);

         _radius += PercentY * 5.0;

         updatePortal = True;
      }

      const Int32 ZoomInt = Value.get_scroll_delta_y ();

      if (ZoomInt != 0) {

         _radius += Float64 (ZoomInt) * 0.2;
         updatePortal = True;
      }

      if (updatePortal) { _update_portal (); }
   }
}


void
dmz::ViewerPluginMenu::on_openAction_triggered () {

   const QString FileName = QFileDialog::getOpenFileName (
      _mainWindowModule ? _mainWindowModule->get_qt_main_window () : 0,
      tr ("Load File"),
      _get_last_path (),
      QString ("*.ive | *.dae"));

   if (!FileName.isEmpty ()) { _load_node_file (FileName); }
}


void
dmz::ViewerPluginMenu::_load_node_file (const QString &FileName) {

   QFileInfo fi (FileName);
   if (fi.exists () && _core) {

      qApp->setOverrideCursor (QCursor (Qt::BusyCursor));

      osg::ref_ptr<osg::Group> scene = _core->get_scene ();

      if (scene.valid ()) {

         if (_node.valid ()) {

            scene->removeChild (_node.get ());
            _node = 0;
         }

         _nodeFile = qPrintable (fi.filePath ());
         _node = osgDB::readNodeFile (_nodeFile.get_buffer ());

         if (_node.valid ()) {

            _appState.set_default_directory (_nodeFile);

            scene->addChild (_node);

            _reset_portal ();
         }
         else {

            _log.error << "Unable to load file: " << _nodeFile << endl;
            _nodeFile.flush ();
         }
      }

      qApp->restoreOverrideCursor ();
   }
}


void
dmz::ViewerPluginMenu::_reset_portal () {

   if (_node.valid ()) {

      osg::BoundingSphere bound = _node->computeBound ();
      osg::Vec3 center = bound.center ();
      _center.set_xyz (center.x (), center.z (), -center.y ());
      _radius = bound.radius () + 2.0;
      _heading = 1.25 * Pi64;
      _pitch = -Pi64 / 8.0;
      _update_portal ();
   }
}


void
dmz::ViewerPluginMenu::_update_portal () {

   if (_portal) {

      Matrix hmat (Vector (0.0, 1.0, 0.0), _heading);
      Matrix pmat (Vector (1.0, 0.0, 0.0), _pitch);
      Matrix dmat = hmat * pmat;
      Vector dir (0.0, 0.0, _radius);
      dmat.transform_vector (dir);

      _portal->set_view (_center + dir, dmat);
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

   _mainWindowModuleName = config_to_string ("module.main-window.name", local);

   Config menuList;
   if (local.lookup_all_config ("menu", menuList)) { _init_menu_list (menuList); }

   QMetaObject::connectSlotsByName (this);

   activate_default_input_channel (InputEventMouseMask);
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
