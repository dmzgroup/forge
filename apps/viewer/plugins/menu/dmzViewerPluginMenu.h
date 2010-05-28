#ifndef DMZ_VIEWER_PLUGIN_MENU_DOT_H
#define DMZ_VIEWER_PLUGIN_MENU_DOT_H

#include <dmzApplicationState.h>
#include <dmzRuntimeLog.h>
#include <dmzRuntimePlugin.h>
#include <dmzRuntimeObjectType.h>
#include <dmzTypesHashTableStringTemplate.h>
#include <QtCore/QObject>
#include <QtGui/QAction>


namespace dmz {

   class ObjectModule;
   class QtModuleMainWindow;


   class ViewerPluginMenu :
         public QObject,
         public Plugin {

      Q_OBJECT

      public:
         ViewerPluginMenu (const PluginInfo &Info, Config &local);
         ~ViewerPluginMenu ();

         // Plugin Interface
         virtual void update_plugin_state (
            const PluginStateEnum State,
            const UInt32 Level);

         virtual void discover_plugin (
            const PluginDiscoverEnum Mode,
            const Plugin *PluginPtr);

      protected Q_SLOTS:
         void on_openAction_triggered ();

      protected:
         struct MenuStruct;
         void _open_file (const QString &FileName);
         QString _get_last_path ();
         void _init_action_list (Config &actionList, MenuStruct &ms);
         void _init_menu_list (Config &menuList);
         void _init (Config &local);

      protected:
         struct MenuStruct {

            const String Name;
            QList<QAction *> actionList;

            MenuStruct (const String &TheName) : Name (TheName) {;}
         };
         
         Log _log;
         ApplicationState _appState;
         ObjectModule *_objectModule;
         QtModuleMainWindow *_mainWindowModule;
         String _mainWindowModuleName;
         HashTableStringTemplate<MenuStruct> _menuTable;
         ObjectType _type;
         Handle _model3dAttrHandle;

      private:
         ViewerPluginMenu ();
         ViewerPluginMenu (const ViewerPluginMenu &);
         ViewerPluginMenu &operator= (const ViewerPluginMenu &);
   };
};


#endif // DMZ_VIEWER_PLUGIN_MENU_DOT_H
