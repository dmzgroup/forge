#ifndef DMZ_FORGE_UPLOADER_PLUGIN_MENU_DOT_H
#define DMZ_FORGE_UPLOADER_PLUGIN_MENU_DOT_H

#include <dmzApplicationState.h>
#include <dmzInputObserverUtil.h>
#include <dmzRuntimeLog.h>
#include <dmzRuntimeMessaging.h>
#include <dmzRuntimePlugin.h>
#include <dmzTypesHashTableStringTemplate.h>
#include <dmzTypesVector.h>
#include <osg/Node>
#include <QtCore/QObject>
#include <QtGui/QAction>


namespace dmz {

   class RenderModuleCoreOSG;
   class RenderModulePortal;
   class QtModuleMainWindow;

   class ForgeUploaderPluginMenu :
         public QObject,
         public Plugin,
         public InputObserverUtil {

      Q_OBJECT

      public:
         ForgeUploaderPluginMenu (const PluginInfo &Info, Config &local);
         ~ForgeUploaderPluginMenu ();

         // Plugin Interface
         virtual void update_plugin_state (
            const PluginStateEnum State,
            const UInt32 Level);

         virtual void discover_plugin (
            const PluginDiscoverEnum Mode,
            const Plugin *PluginPtr);

         // Input Observer Util Interface
         virtual void receive_mouse_event (
            const Handle Channel,
            const InputEventMouse &Value);

      protected Q_SLOTS:
         void on_openAction_triggered ();

      protected:
         struct MenuStruct;
         void _load_node_file (const QString &FileName);
         void _reset_portal ();
         void _update_portal ();
         QString _get_last_path ();
         void _init_action_list (Config &actionList, MenuStruct &ms);
         void _init_menu_list (Config &menuList);
         void _init (Config &local);

      protected:
         Log _log;
         ApplicationState _appState;
         QtModuleMainWindow *_mainWindowModule;
         String _mainWindowModuleName;
         RenderModuleCoreOSG *_core;
         RenderModulePortal *_portal;

         struct MenuStruct {

            const String Name;
            QList<QAction *> actionList;

            MenuStruct (const String &TheName) : Name (TheName) {;}
         };

         HashTableStringTemplate<MenuStruct> _menuTable;

         osg::ref_ptr<osg::Node> _node;
         String _nodeFile;
         Vector _center;
         Float64 _radius;
         Float64 _heading;
         Float64 _pitch;

      private:
         ForgeUploaderPluginMenu ();
         ForgeUploaderPluginMenu (const ForgeUploaderPluginMenu &);
         ForgeUploaderPluginMenu &operator= (const ForgeUploaderPluginMenu &);
   };
};


#endif // DMZ_FORGE_UPLOADER_PLUGIN_MENU_DOT_H
