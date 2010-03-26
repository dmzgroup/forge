#ifndef DMZ_FORGE_QT_PLUGIN_CLIENT_DOT_H
#define DMZ_FORGE_QT_PLUGIN_CLIENT_DOT_H

#include <dmzRuntimePlugin.h>
#include <dmzRuntimeMessaging.h>
#include <QtCore/QObject>

namespace dmz {

   class ForgeQtWebService;

   class ForgeQtPluginClient : public QObject, public Plugin {

      Q_OBJECT
      
      public:
         ForgeQtPluginClient (const PluginInfo &Info, Config &local);
         ~ForgeQtPluginClient ();

         // Plugin Interface
         virtual void update_plugin_state (
            const PluginStateEnum State,
            const UInt32 Level);

         virtual void discover_plugin (
            const PluginDiscoverEnum Mode,
            const Plugin *PluginPtr);
            
      protected Q_SLOTS:
         void _handle_asset_list (const StringContainer &Results);
         void _handle_asset (const Config &Results);

      protected:
         void _init (Config &local);

         struct State;
         State &_state;
         
      private:
         ForgeQtPluginClient ();
         ForgeQtPluginClient (const ForgeQtPluginClient &);
         ForgeQtPluginClient &operator= (const ForgeQtPluginClient &);
   };
};


#endif // DMZ_FORGE_QT_PLUGIN_CLIENT_DOT_H
