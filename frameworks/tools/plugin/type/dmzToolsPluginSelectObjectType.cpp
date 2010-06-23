#include <dmzQtConfigRead.h>
#include <dmzQtConfigWrite.h>
#include <dmzRuntimeConfigWrite.h>
#include <dmzRuntimePluginFactoryLinkSymbol.h>
#include <dmzRuntimePluginInfo.h>
#include <dmzRuntimeSession.h>
#include "dmzToolsPluginSelectObjectType.h"

dmz::ToolsPluginSelectObjectType::ToolsPluginSelectObjectType (
      const PluginInfo &Info,
      Config &local) :
      Plugin (Info),
      MessageObserver (Info),
      _log (Info),
      _defs (Info),
      _convert (Info) {

   _ui.setupUi (this);

   show ();

   _init (local);
}


dmz::ToolsPluginSelectObjectType::~ToolsPluginSelectObjectType () {

}


// Plugin Interface
void
dmz::ToolsPluginSelectObjectType::update_plugin_state (
      const PluginStateEnum State,
      const UInt32 Level) {

   if (State == PluginStateInit) {

   }
   else if (State == PluginStateStart) {

      if (!_currentTypeName) {

         on_typeList_itemClicked (_ui.typeList->item (0));
      }
   }
   else if (State == PluginStateStop) {

   }
   else if (State == PluginStateShutdown) {

      RuntimeContext *context (get_plugin_runtime_context ());

      if (context) {

         Config session (get_plugin_name ());

         session.add_config (qbytearray_to_config ("geometry", saveGeometry ()));

         if (isVisible ()) {

            session.add_config (boolean_to_config ("window", "visible", True));
         }

         set_session_config (context, session);
      }
   }
}


void
dmz::ToolsPluginSelectObjectType::discover_plugin (
      const PluginDiscoverEnum Mode,
      const Plugin *PluginPtr) {

   if (Mode == PluginDiscoverAdd) {

   }
   else if (Mode == PluginDiscoverRemove) {

   }
}


// Message Observer Interface
void
dmz::ToolsPluginSelectObjectType::receive_message (
      const Message &Type,
      const UInt32 MessageSendHandle,
      const Handle TargetObserverHandle,
      const Data *InData,
      Data *outData) {

   const String TypeName = _convert.to_string (InData);

   if (TypeName && (TypeName != _currentTypeName)) {

      QList<QListWidgetItem *> list =
         _ui.typeList->findItems (TypeName.get_buffer (), Qt::MatchExactly);

      if (!list.isEmpty ()) {

         QListWidgetItem *item = list.first ();

         if (item) {

            _ui.typeList->setCurrentItem (item);
            _currentTypeName = TypeName;
            _ui.typeLabel->setText (_currentTypeName.get_buffer ());
         }
      }
   }
}


// ToolsPluginSelectObjectType Interface
void
dmz::ToolsPluginSelectObjectType::on_typeList_itemClicked (QListWidgetItem *item) {

   if (item) {

      String TypeName = qPrintable (item->text ());

      if (TypeName != _currentTypeName) {

         Data out = _convert.to_data (TypeName);
         _typeMsg.send (&out);
      }
   }
}

void
dmz::ToolsPluginSelectObjectType::_add_type (const ObjectType &Type) {

   _ui.typeList->addItem (Type.get_name ().get_buffer ());

   ObjectTypeIterator it;
   ObjectType next;

   while (Type.get_next_child (it, next)) { _add_type (next); }
}


void
dmz::ToolsPluginSelectObjectType::_init (Config &local) {

   RuntimeContext *context = get_plugin_runtime_context ();

   if (context) {

      Config session (get_session_config (get_plugin_name (), context));

      Config gdata;

      if (session.lookup_config ("geometry", gdata)) {

         restoreGeometry (config_to_qbytearray (gdata));
      }

      if (config_to_boolean ("window.visible", session, False)) { show (); }
   }

   _add_type (_defs.get_root_object_type ());

   _typeMsg = config_create_monostate_message (
      "message.name",
      local,
      "Tools_Current_Object_Type_Message",
      context);

   subscribe_to_message (_typeMsg);
}


extern "C" {

DMZ_PLUGIN_FACTORY_LINK_SYMBOL dmz::Plugin *
create_dmzToolsPluginSelectObjectType (
      const dmz::PluginInfo &Info,
      dmz::Config &local,
      dmz::Config &global) {

   return new dmz::ToolsPluginSelectObjectType (Info, local);
}

};
