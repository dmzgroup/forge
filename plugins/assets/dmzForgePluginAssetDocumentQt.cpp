#include "dmzForgePluginAssetDocumentQt.h"
#include <dmzFoundationConfigFileIO.h>
#include <dmzQtUtil.h>
#include <dmzSystemFile.h>
#include <dmzRuntimeConfigToTypesBase.h>
#include <dmzRuntimePluginFactoryLinkSymbol.h>
#include <dmzRuntimePluginInfo.h>
#include <dmzRuntimeSession.h>

dmz::ForgePluginAssetDocumentQt::ForgePluginAssetDocumentQt (
      const PluginInfo &Info,
      Config &local) :
      Plugin (Info),
      MessageObserver (Info),
      _log (Info),
      _converter (Info) {

   _ui.setupUi (this);

   hide ();

   _init (local);
}


dmz::ForgePluginAssetDocumentQt::~ForgePluginAssetDocumentQt () {

}


// Plugin Interface
void
dmz::ForgePluginAssetDocumentQt::update_plugin_state (
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
dmz::ForgePluginAssetDocumentQt::discover_plugin (
      const PluginDiscoverEnum Mode,
      const Plugin *PluginPtr) {

   if (Mode == PluginDiscoverAdd) {

   }
   else if (Mode == PluginDiscoverRemove) {

   }
}


// Message Observer Interface
void
dmz::ForgePluginAssetDocumentQt::receive_message (
      const Message &Type,
      const UInt32 MessageSendHandle,
      const Handle TargetObserverHandle,
      const Data *InData,
      Data *outData) {

   if (Type == _processMsg) {

      const String FileName = _converter.to_string (InData);

      if (FileName) {

         _init_ui (FileName);

         _ui.fileLabel->setText (FileName.get_buffer ());
         show ();
         activateWindow ();
      }
   }
}


void
dmz::ForgePluginAssetDocumentQt::on_buttonBox_accepted () {

   hide ();
   _finishedMsg.send ();
}


void
dmz::ForgePluginAssetDocumentQt::on_buttonBox_rejected () {

   hide ();
   _finishedMsg.send ();
}


void
dmz::ForgePluginAssetDocumentQt::_init_ui (const String &FileName) {

   _currentConfig.set_config_context (0);

   _currentConfigFile = FileName + ".json";

   if (is_valid_path (_currentConfigFile)) {

      read_config_file (_currentConfigFile, _currentConfig, FileTypeAutoDetect, &_log);
   }

_log.warn << _currentConfig << endl;

   const String Name = config_to_string ("dmz.name", _currentConfig, "");
   _ui.nameEdit->setText (Name.get_buffer ());
   const String Brief = config_to_string ("dmz.brief", _currentConfig, "");
   _ui.briefEdit->setText (Brief.get_buffer ());
   const String Details = config_to_string ("dmz.details", _currentConfig, "");
   _ui.detailsEdit->setPlainText (Details.get_buffer ());
}


void
dmz::ForgePluginAssetDocumentQt::_init (Config &local) {

   RuntimeContext *context (get_plugin_runtime_context ());

   Config session (get_session_config (get_plugin_name (), context));

   QByteArray geometry (config_to_qbytearray ("geometry", session, saveGeometry ()));
   restoreGeometry (geometry);

   _processMsg = config_create_message (
      "process-file-message.name",
      local,
      "ProcessFileMessage",
      get_plugin_runtime_context ());

   subscribe_to_message (_processMsg);

   _finishedMsg = config_create_message (
      "finished-processing-file-message.name",
      local,
      "FinishedProcessingFileMessage",
      get_plugin_runtime_context ());
}


extern "C" {

DMZ_PLUGIN_FACTORY_LINK_SYMBOL dmz::Plugin *
create_dmzForgePluginAssetDocumentQt (
      const dmz::PluginInfo &Info,
      dmz::Config &local,
      dmz::Config &global) {

   return new dmz::ForgePluginAssetDocumentQt (Info, local);
}

};
