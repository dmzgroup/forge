#ifndef DMZ_FORGE_PLUGIN_PUBLISH_DOT_H
#define DMZ_FORGE_PLUGIN_PUBLISH_DOT_H

#include "AssetPublisher.h"
#include <dmzForgeObserver.h>
#include <dmzQtWidget.h>
#include <dmzRuntimeMessaging.h>
#include <dmzRuntimePlugin.h>
#include <dmzRuntimeTimeSlice.h>
#include <QtGui/QFrame>


namespace dmz {

   class ForgePluginPublish :
      public QFrame,
      public QtWidget,
      public Plugin,
      public TimeSlice,
      public MessageObserver,
      public ForgeObserver {

      Q_OBJECT

      public:
         ForgePluginPublish (const PluginInfo &Info, Config &local);
         ~ForgePluginPublish ();

         // QtWidget Interface
         virtual QWidget *get_qt_widget ();

         // Plugin Interface
         virtual void update_plugin_state (
            const PluginStateEnum State,
            const UInt32 Level);

         virtual void discover_plugin (
            const PluginDiscoverEnum Mode,
            const Plugin *PluginPtr);

         // TimeSlice Interface
         virtual void update_time_slice (const Float64 TimeDelta);

         // Message Observer Interface
         virtual void receive_message (
            const Message &Type,
            const Handle MessageSendHandle,
            const Handle TargetObserverHandle,
            const Data *InData,
            Data *outData);

         // ForgeObserver Interface
         virtual void handle_reply (
            const UInt64 RequestId,
            const Int32 ReqeustType,
            const Boolean Error,
            const StringContainer &Results);

      protected Q_SLOTS:
         void on_cancelButton_clicked ();
         void on_updatePreviewsButton_clicked ();
         void _slot_publish ();
         void _slot_reset ();

      protected:
         Boolean _dump_model (const String &File);
         void _init (Config &local);

         AssetPublisher _asset;

         struct State;
         State &_state;

      private:
         ForgePluginPublish ();
         ForgePluginPublish (const ForgePluginPublish &);
         ForgePluginPublish &operator= (const ForgePluginPublish &);
   };
};


#endif // DMZ_FORGE_PLUGIN_PUBLISH_DOT_H
