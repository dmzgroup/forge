#ifndef DMZ_WEB_SERVICES_PLUGIN_AUTO_FETCH_DOT_H
#define DMZ_WEB_SERVICES_PLUGIN_AUTO_FETCH_DOT_H

#include <dmzRuntimeLog.h>
#include <dmzRuntimePlugin.h>

namespace dmz {

   class WebServicesPluginAutoFetch :
         public Plugin {

      public:
         WebServicesPluginAutoFetch (const PluginInfo &Info, Config &local);
         ~WebServicesPluginAutoFetch ();

         // Plugin Interface
         virtual void update_plugin_state (
            const PluginStateEnum State,
            const UInt32 Level);

         virtual void discover_plugin (
            const PluginDiscoverEnum Mode,
            const Plugin *PluginPtr);

      protected:
         // WebServicesPluginAutoFetch Interface
         void _init (Config &local);

         Log _log;

      private:
         WebServicesPluginAutoFetch ();
         WebServicesPluginAutoFetch (const WebServicesPluginAutoFetch &);
         WebServicesPluginAutoFetch &operator= (const WebServicesPluginAutoFetch &);

   };
};

#endif // DMZ_WEB_SERVICES_PLUGIN_AUTO_FETCH_DOT_H
