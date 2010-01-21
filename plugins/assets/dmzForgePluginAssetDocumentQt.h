#ifndef DMZ_FORGE_PLUGIN_ASSET_DOCUMENT_QT_DOT_H
#define DMZ_FORGE_PLUGIN_ASSET_DOCUMENT_QT_DOT_H

#include <dmzRuntimeConfig.h>
#include <dmzRuntimeDataConverterStringContainer.h>
#include <dmzRuntimeDataConverterTypesBase.h>
#include <dmzRuntimeLog.h>
#include <dmzRuntimeMessaging.h>
#include <dmzRuntimePlugin.h>
#include <dmzRuntimeTimeSlice.h>

#include <QtGui/QWidget>

#include <ui_AssetDocument.h>

namespace dmz {

   class ForgePluginAssetDocumentQt :
         public QWidget,
         public Plugin,
         public TimeSlice,
         public MessageObserver {

      Q_OBJECT

      public:
         ForgePluginAssetDocumentQt (const PluginInfo &Info, Config &local);
         ~ForgePluginAssetDocumentQt ();

         // Plugin Interface
         virtual void update_plugin_state (
            const PluginStateEnum State,
            const UInt32 Level);

         virtual void discover_plugin (
            const PluginDiscoverEnum Mode,
            const Plugin *PluginPtr);

         // Time Slice Interface
         virtual void update_time_slice (const Float64 TimeDelta);

         // Message Observer Interface
         virtual void receive_message (
            const Message &Type,
            const UInt32 MessageSendHandle,
            const Handle TargetObserverHandle,
            const Data *InData,
            Data *outData);

         void closeEvent (QCloseEvent *);

      protected slots:
         void on_buttonBox_accepted ();
         void on_buttonBox_rejected ();
         void on_ignoreButton_pressed ();
         void on_clearButton_pressed ();
         void on_imageButton_pressed ();

      protected:
         void _update_thumbnails (const StringContainer &List);
         void _save_info ();
         void _init_ui (const String &FileName);
         void _init (Config &local);

         Log _log;

         Ui::AssetForm _ui;

         Boolean _finished;

         String _currentFile;
         String _currentConfigFile;
         Config _currentConfig;

         String _configSubPath;

         Message _processMsg;
         Message _finishedMsg;
         Message _startScreenMsg;
         Message _doneScreenMsg;
         DataConverterString _convert;
         DataConverterStringContainer _listConvert;

      private:
         ForgePluginAssetDocumentQt ();
         ForgePluginAssetDocumentQt (const ForgePluginAssetDocumentQt &);
         ForgePluginAssetDocumentQt &operator= (const ForgePluginAssetDocumentQt &);

   };
};

#endif // DMZ_FORGE_PLUGIN_ASSET_DOCUMENT_QT_DOT_H
