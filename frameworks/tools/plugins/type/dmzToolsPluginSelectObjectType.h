#ifndef DMZ_TOOLS_PLUGIN_SELECT_OBJECT_TYPE_DOT_H
#define DMZ_TOOLS_PLUGIN_SELECT_OBJECT_TYPE_DOT_H

#include <dmzRuntimeDataConverterTypesBase.h>
#include <dmzRuntimeDefinitions.h>
#include <dmzRuntimeLog.h>
#include <dmzRuntimeMessaging.h>
#include <dmzRuntimeObjectType.h>
#include <dmzRuntimePlugin.h>

#include <QtGui/QFrame>
#include <ui_ObjectTypeSelector.h>

namespace dmz {

   class ToolsPluginSelectObjectType :
         public QFrame,
         public Plugin,
         public MessageObserver {

      Q_OBJECT

      public:
         ToolsPluginSelectObjectType (const PluginInfo &Info, Config &local);
         ~ToolsPluginSelectObjectType ();

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

      protected slots:
         void on_typeList_itemClicked (QListWidgetItem *item);

      protected:
         // ToolsPluginSelectObjectType Interface
         void _add_type (const ObjectType &Type);
         void _init (Config &local);

         Log _log;
         Definitions _defs;
         DataConverterString _convert;

         String _currentTypeName;

         Message _typeMsg;

         Ui::ObjectTypeSelector _ui;

      private:
         ToolsPluginSelectObjectType ();
         ToolsPluginSelectObjectType (const ToolsPluginSelectObjectType &);
         ToolsPluginSelectObjectType &operator= (const ToolsPluginSelectObjectType &);

   };
};

#endif // DMZ_TOOLS_PLUGIN_SELECT_OBJECT_TYPE_DOT_H
