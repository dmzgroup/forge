#include <dmzRuntimeConfigToTypesBase.h>
#include <dmzRuntimePluginFactoryLinkSymbol.h>
#include <dmzRuntimePluginInfo.h>
#include <dmzWebServicesModule.h>
#include "dmzWebServicesPluginAutoTrackUndo.h"


dmz::WebServicesPluginAutoTrackUndo::WebServicesPluginAutoTrackUndo (const PluginInfo &Info, Config &local) :
      Plugin (Info),
      UndoObserver (Info),
      _log (Info),
      _undo (Info),
      _webservices (0),
      _sessionHandle (0) {

   _init (local);
}


dmz::WebServicesPluginAutoTrackUndo::~WebServicesPluginAutoTrackUndo () {

}


// Plugin Interface
void
dmz::WebServicesPluginAutoTrackUndo::update_plugin_state (
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
dmz::WebServicesPluginAutoTrackUndo::discover_plugin (
      const PluginDiscoverEnum Mode,
      const Plugin *PluginPtr) {

   if (Mode == PluginDiscoverAdd) {

      if (!_webservices) {

         _webservices = WebServicesModule::cast (PluginPtr, _webservicesName);
      }
   }
   else if (Mode == PluginDiscoverRemove) {

      if (_webservices && (_webservices == WebServicesModule::cast (PluginPtr))) {

         _webservices = 0;
      }
   }
}


// Undo Observer Interface
void
dmz::WebServicesPluginAutoTrackUndo::update_recording_state (
      const UndoRecordingStateEnum RecordingState,
      const UndoRecordingTypeEnum RecordingType,
      const UndoTypeEnum UndoType) {

   if (_webservices) {

      if (RecordingState == UndoRecordingStateStart) {

         String nextUndo, nextRedo;
         _undo.get_current_undo_names (nextUndo, nextRedo);
//         _sessionHandle = _webservices->start_session (nextUndo);
//         _log.error << "Start: " << nextUndo << " : " << _sessionHandle << endl;
      }
      else if (RecordingState == UndoRecordingStateStop) {

//         _webservices->stop_session (_sessionHandle);
         _sessionHandle = 0;
      }
      else if (RecordingState == UndoRecordingStateAbort) {

//         _webservices->abort_session (_sessionHandle);
         _sessionHandle = 0;
      }
   }
}


void
dmz::WebServicesPluginAutoTrackUndo::update_current_undo_names (
      const String *NextUndoName,
      const String *NextRedoName) {

}


// WebServicesPluginAutoTrackUndo Interface
void
dmz::WebServicesPluginAutoTrackUndo::_init (Config &local) {

   _webservicesName = config_to_string ("module-name.web-services", local);
}


extern "C" {

DMZ_PLUGIN_FACTORY_LINK_SYMBOL dmz::Plugin *
create_dmzWebServicesPluginAutoTrackUndo (
      const dmz::PluginInfo &Info,
      dmz::Config &local,
      dmz::Config &global) {

   return new dmz::WebServicesPluginAutoTrackUndo (Info, local);
}

};
