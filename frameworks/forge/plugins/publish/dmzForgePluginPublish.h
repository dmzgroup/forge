#ifndef DMZ_FORGE_PLUGIN_PUBLISH_DOT_H
#define DMZ_FORGE_PLUGIN_PUBLISH_DOT_H

#include <dmzForgeObserver.h>
#include <dmzRuntimePlugin.h>
#include <dmzRuntimeTimeSlice.h>
#include <QtGui/QFrame>
#include <ui_PublishForm.h>


namespace dmz {

   class ForgePluginPublish :
      public QFrame,
      public Plugin,
      public TimeSlice,
      public ForgeObserver {

      Q_OBJECT
      
      public:
         ForgePluginPublish (const PluginInfo &Info, Config &local);
         ~ForgePluginPublish ();

         // Plugin Interface
         virtual void update_plugin_state (
            const PluginStateEnum State,
            const UInt32 Level);

         virtual void discover_plugin (
            const PluginDiscoverEnum Mode,
            const Plugin *PluginPtr);
         
         // TimeSlice Interface
         virtual void update_time_slice (const Float64 TimeDelta);
         
         // ForgeObserver Interface
         virtual void handle_reply (
            const UInt64 RequestId,
            const Int32 ReqeustType,
            const Boolean Error,
            const StringContainer &Results);
         
      protected:
         void _init (Config &local);

         struct State;
         State &_state;

         Ui::PublishForm _ui;
         
      private:
         ForgePluginPublish ();
         ForgePluginPublish (const ForgePluginPublish &);
         ForgePluginPublish &operator= (const ForgePluginPublish &);
   };
};


#endif // DMZ_FORGE_PLUGIN_PUBLISH_DOT_H
