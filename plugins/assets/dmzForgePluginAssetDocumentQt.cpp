#include "dmzForgePluginAssetDocumentQt.h"
#include <dmzFoundationConfigFileIO.h>
#include <dmzFoundationJSONUtil.h>
#include <dmzQtUtil.h>
#include <dmzSystemFile.h>
#include <dmzSystemStreamString.h>
#include <dmzRuntimeConfigToTypesBase.h>
#include <dmzRuntimePluginFactoryLinkSymbol.h>
#include <dmzRuntimePluginInfo.h>
#include <dmzRuntimeSession.h>
#include <dmzTypesStringTokenizer.h>
#include <dmzTypesStringUtil.h>

namespace {

static const dmz::String LocalName ("name");
static const dmz::String LocalBrief ("brief");
static const dmz::String LocalDetails ("details");
static const dmz::String LocalKeywords ("keywords");
static const dmz::String LocalValue ("value");

};

dmz::ForgePluginAssetDocumentQt::ForgePluginAssetDocumentQt (
      const PluginInfo &Info,
      Config &local) :
      Plugin (Info),
      MessageObserver (Info),
      _log (Info),
      _converter (Info),
      _saveInfo (False) {

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

      Config session (get_plugin_name ());

      session.add_config (qbytearray_to_config ("geometry", saveGeometry ()));

      set_session_config (get_plugin_runtime_context (), session);
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

   _saveInfo = True;
   close ();
}


void
dmz::ForgePluginAssetDocumentQt::on_buttonBox_rejected () {

   _saveInfo = False;
   close ();
}


void
dmz::ForgePluginAssetDocumentQt::closeEvent (QCloseEvent *) {

   if (_saveInfo) {

      _currentConfig.store_attribute (LocalName, qPrintable (_ui.nameEdit->text ()));
      _currentConfig.store_attribute (LocalBrief, qPrintable (_ui.briefEdit->text ()));

      _currentConfig.store_attribute (
         LocalDetails,
         qPrintable (_ui.detailsEdit->toPlainText ()));

      const String Keywords = qPrintable (_ui.keywordEdit->text ());

      StringTokenizer stz (Keywords, ',');
      Boolean first = True;
      String value;

      while (stz.get_next (value)) {

         trim_ascii_white_space (value);

         Config data (LocalKeywords);
         data.store_attribute (LocalValue, value);

         if (first) { _currentConfig.overwrite_config (data); first = False; }
         else { _currentConfig.add_config (data); }
      }

      String outStr;
      StreamString out (outStr);
      format_config_to_json (_currentConfig, out, ConfigStripGlobal | ConfigPrettyPrint, &_log);

_log.error << outStr << endl;
   }

   _saveInfo = False;
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

   const String Name = config_to_string (LocalName, _currentConfig, "");
   _ui.nameEdit->setText (Name.get_buffer ());
   const String Brief = config_to_string (LocalBrief, _currentConfig, "");
   _ui.briefEdit->setText (Brief.get_buffer ());
   const String Details = config_to_string (LocalDetails, _currentConfig, "");
   _ui.detailsEdit->setPlainText (Details.get_buffer ());
   Config keywordList;
   String keywords;

   if (_currentConfig.lookup_all_config (LocalKeywords, keywordList)) {

      ConfigIterator it;
      Config word;

      while (keywordList.get_next_config (it, word)) {

         const String Value = config_to_string (LocalValue, word);

         if (Value) {

            if (keywords) { keywords << ", ";}
            keywords << Value;
         }
      }

      _ui.keywordEdit->setText (keywords.get_buffer ());
   }
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
      "Process_File_Message",
      get_plugin_runtime_context ());

   subscribe_to_message (_processMsg);

   _finishedMsg = config_create_message (
      "finished-processing-file-message.name",
      local,
      "Finished_Processing_File_Message",
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
