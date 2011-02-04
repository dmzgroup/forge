#ifndef DMZ_WEB_SERVICES_PLUGIN_AUTO_TRACK_UNDO_DOT_H
#define DMZ_WEB_SERVICES_PLUGIN_AUTO_TRACK_UNDO_DOT_H

#include <dmzRuntimeLog.h>
#include <dmzRuntimePlugin.h>
#include <dmzRuntimeUndo.h>

namespace dmz {

   class WebServicesModule;

   class WebServicesPluginAutoTrackUndo :
         public Plugin,
         public UndoObserver {

      public:
         WebServicesPluginAutoTrackUndo (const PluginInfo &Info, Config &local);
         ~WebServicesPluginAutoTrackUndo ();

         // Plugin Interface
         virtual void update_plugin_state (
            const PluginStateEnum State,
            const UInt32 Level);

         virtual void discover_plugin (
            const PluginDiscoverEnum Mode,
            const Plugin *PluginPtr);

         // Undo Observer Interface
         virtual void update_recording_state (
            const UndoRecordingStateEnum RecordingState,
            const UndoRecordingTypeEnum RecordingType,
            const UndoTypeEnum UndoType);

         virtual void update_current_undo_names (
            const String *NextUndoName,
            const String *NextRedoName);

      protected:
         // WebServicesPluginAutoTrackUndo Interface
         void _init (Config &local);

         Log _log;
         Undo _undo;
         WebServicesModule *_webservices;
         String _webservicesName;
         Handle _sessionHandle;

      private:
         WebServicesPluginAutoTrackUndo ();
         WebServicesPluginAutoTrackUndo (const WebServicesPluginAutoTrackUndo &);
         WebServicesPluginAutoTrackUndo &operator= (const WebServicesPluginAutoTrackUndo &);

   };
};

#endif // DMZ_WEB_SERVICES_PLUGIN_AUTO_TRACK_UNDO_DOT_H
