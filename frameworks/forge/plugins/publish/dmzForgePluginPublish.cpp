#include <dmzForgeConsts.h>
#include <dmzForgeModule.h>
#include "dmzForgePluginPublish.h"
#include <dmzObjectModule.h>
#include <dmzQtConfigRead.h>
#include <dmzRenderModuleCoreOSG.h>
#include <dmzRuntimeConfig.h>
#include <dmzRuntimeConfigToNamedHandle.h>
#include <dmzRuntimeConfigToTypesBase.h>
#include <dmzRuntimeDataConverterStringContainer.h>
#include <dmzRuntimeDataConverterTypesBase.h>
#include <dmzRuntimeLog.h>
#include <dmzRuntimePluginFactoryLinkSymbol.h>
#include <dmzRuntimePluginInfo.h>
#include <dmzRuntimeUUID.h>
#include <dmzSystemFile.h>
#include <dmzTypesStringContainer.h>
#include <QtGui/QtGui>
#include <osgDB/WriteFile>


struct dmz::ForgePluginPublish::State {

   Log log;
   DataConverterHandle convertHandle;
   DataConverterString convertString;
   DataConverterStringContainer convertList;
   ObjectModule *objectModule;
   ForgeModule *forgeModule;
   RenderModuleCoreOSG *core;
   String forgeModuleName;
   Handle modelAttrHandle;
   Handle target;
   String modelSource;
   String modelTarget;
   String targetBase;
   Boolean createTarget;
   Message attachMsg;
   Message startCaptureMsg;
   Message finishedCaptureMsg;
   Message cleanupMsg;
   Message loadAssetMsg;
   StringContainer previews;
   StringContainerIterator previewIt;
   String currentPreview;
   UInt32 currentPreviewIndex;
   Boolean newPreviews;

   State (const PluginInfo &Info);
   ~State ();
};


dmz::ForgePluginPublish::State::State (const PluginInfo &Info) :
      log (Info),
      convertHandle (Info),
      convertString (Info),
      convertList (Info),
      objectModule (0),
      forgeModule (0),
      core (0),
      modelAttrHandle (0),
      target (0),
      createTarget (False),
      currentPreviewIndex (1),
      newPreviews (False) {

   const UUID RuntimeId (get_runtime_uuid (Info));

   targetBase = qPrintable (QDir::tempPath ());
   targetBase = format_path (targetBase + "/" + RuntimeId.to_string ());

   // create runtime temp directory
   if (!is_valid_path (targetBase)) {

      create_directory (targetBase);
      log.info << "Created runtime temp dir: " << targetBase << endl;
   }
}


dmz::ForgePluginPublish::State::~State () {

   String file;
   StringContainer fileList;
   get_file_list (targetBase, fileList);

   while (fileList.get_next (file)) {

      file = format_path (targetBase + "/" + file);
      remove_file (file);
   }

   remove_file (targetBase);
}


dmz::ForgePluginPublish::ForgePluginPublish (const PluginInfo &Info, Config &local) :
      QFrame (0),
      QtWidget (Info),
      Plugin (Info),
      TimeSlice (Info, TimeSliceTypeSystemTime, TimeSliceModeSingle, 1.0),
      MessageObserver (Info),
      ForgeObserver (Info),
      _asset (Info),
      _state (*(new State (Info))) {

   _asset.setupUi (this);
   _asset.reset ();

   QPushButton *button = _asset.buttonBox->button (QDialogButtonBox::Apply);
   if (button) {

      button->setDefault (True);
      button->setText (QLatin1String ("Publish"));

      connect (button, SIGNAL (clicked ()), this, SLOT (_slot_publish ()));
   }

   button = _asset.buttonBox->button (QDialogButtonBox::Reset);
   if (button) {

      connect (button, SIGNAL (clicked ()), this, SLOT (_slot_reset ()));
   }

   _init (local);

   QPixmapCache::setCacheLimit (1024 * 1024);
}


dmz::ForgePluginPublish::~ForgePluginPublish () {

   delete &_state;
}


// QtWidget Interface
QWidget *
dmz::ForgePluginPublish::get_qt_widget () {

   return this;
}


// Plugin Interface
void
dmz::ForgePluginPublish::update_plugin_state (
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
dmz::ForgePluginPublish::discover_plugin (
      const PluginDiscoverEnum Mode,
      const Plugin *PluginPtr) {

   if (Mode == PluginDiscoverAdd) {

      if (!_state.objectModule) { _state.objectModule = ObjectModule::cast (PluginPtr); }

      if (!_state.forgeModule) {

         _state.forgeModule = ForgeModule::cast (PluginPtr, _state.forgeModuleName);

         if (_state.forgeModule) {

            _asset.set_forge_module (_state.forgeModule);
         }
      }

      if (!_state.core) { _state.core = RenderModuleCoreOSG::cast (PluginPtr); }
   }
   else if (Mode == PluginDiscoverRemove) {

      if (_state.objectModule && (_state.objectModule == ObjectModule::cast (PluginPtr))) {

         _state.objectModule = 0;
      }

      if (_state.forgeModule && (_state.forgeModule == ForgeModule::cast (PluginPtr))) {

         _asset.set_forge_module (0);
         _state.forgeModule = 0;
      }

      if (_state.core && (_state.core == RenderModuleCoreOSG::cast (PluginPtr))) {

         _state.core = 0;
      }
   }
}


// TimeSlice Interface
void
dmz::ForgePluginPublish::update_time_slice (const Float64 TimeDelta) {

}


// Message Observer Interface
void
dmz::ForgePluginPublish::receive_message (
      const Message &Type,
      const Handle MessageSendHandle,
      const Handle TargetObserverHandle,
      const Data *InData,
      Data *outData) {

   if (Type == _state.attachMsg) {

      _state.target = _state.convertHandle.to_handle (InData);

      if (_state.target && _state.objectModule) {

         _state.modelSource.flush ();
         _state.modelTarget.flush ();
         _state.createTarget = False;

         if (_state.objectModule->lookup_text (
               _state.target,
               _state.modelAttrHandle,
               _state.modelSource)) {

            QFileInfo fi (_state.modelSource.get_buffer ());

            if (fi.suffix ().toLower () == "ive") {

               _state.modelTarget = _state.modelSource;
               _state.createTarget = False;
            }
            else {

               _state.modelTarget = _state.targetBase;
               _state.modelTarget << "/" << qPrintable (fi.baseName ()) << ".ive";
//               _state.modelTarget.replace_sub (" ", "_");

               _state.createTarget = True;
            }
         }
      }
   }
   else if (Type == _state.finishedCaptureMsg) {

      _state.previews = _state.convertList.to_string_container (InData);
      _state.previews.get_first (_state.previewIt, _state.currentPreview);
      _state.currentPreviewIndex = 1;
      _state.newPreviews = True;
      _update_preview ();
   }
   else if (Type == _state.loadAssetMsg) {

      String assetId = _state.convertString.to_string (InData);
      _asset.set_asset (assetId);
   }
}


// ForgeObserver Interface
void
dmz::ForgePluginPublish::handle_reply (
      const UInt64 RequestId,
      const Int32 ReqeustType,
      const Boolean Error,
      const StringContainer &Results) {

   switch (ReqeustType) {

      case ForgeTypeSearch: {
         String assetId;
         while (Results.get_next (assetId)) {

            // _state.log.info << "getting asset: " << assetId << endl;
            // _state.forgeModule->get_asset (assetId, this);
         }
         break;
      }

      default:
         break;
   }

   if (Error) {

_state.log.error << "Results: " << Results << endl;
   }
}


void
dmz::ForgePluginPublish::on_cancelButton_clicked () {

   _asset.cancel ();
}


void
dmz::ForgePluginPublish::on_updatePreviewsButton_clicked () {

   if (_state.targetBase) {

      const String BaseName (format_path (_state.targetBase + "/preview_"));

      Data out = _state.convertString.to_data (BaseName);
      _state.startCaptureMsg.send (&out);
   }
}


void
dmz::ForgePluginPublish::on_nextPreviewButton_clicked () {

   if (_state.previews.get_next (_state.previewIt, _state.currentPreview)) {

      _state.currentPreviewIndex++;
   }
   else {

      _state.previews.get_first (_state.previewIt, _state.currentPreview);
      _state.currentPreviewIndex = 1;
   }

   _update_preview ();
}


void
dmz::ForgePluginPublish::on_prevPreviewButton_clicked () {

   if (_state.previews.get_prev (_state.previewIt, _state.currentPreview)) {

      _state.currentPreviewIndex--;
   }
   else {

      _state.previews.get_last (_state.previewIt, _state.currentPreview);
      _state.currentPreviewIndex = _state.previews.get_count ();
   }

   _update_preview ();
}


void
dmz::ForgePluginPublish::_slot_publish () {

   if (_state.createTarget) {

      if (_dump_model (_state.modelTarget)) { _state.createTarget = False; }
      else {

_state.log.error << "Failed saving model to file: " << _state.modelTarget << endl;
_state.modelTarget.flush ();
      }
   }

   if (_state.modelTarget) {

      _asset.add_media (_state.modelTarget);
      _state.modelTarget.flush ();
   }

   _asset.publish ();
}


void
dmz::ForgePluginPublish::_slot_reset () {

   _asset.reset ();
   _state.cleanupMsg.send ();
   _state.previews.clear ();
   _state.previewIt.reset ();
   _asset.previewInfoLabel->clear ();
}


void
dmz::ForgePluginPublish::_update_preview () {

   if (_state.newPreviews) {

      QPixmapCache::clear ();
      _asset.add_previews (_state.previews);
      _state.newPreviews = False;
   }

   if (_state.previews.get_count ()) {

      const QString Key = QString ("preview_%1").arg (_state.currentPreviewIndex);

      QPixmap pixmap;
      if (!QPixmapCache::find (Key, &pixmap)) {

         pixmap.load (_state.currentPreview.get_buffer ());

         if ((pixmap.width () > 200) || (pixmap.height () > 200)) {

            pixmap = pixmap.scaled (200, 200, Qt::KeepAspectRatio, Qt::SmoothTransformation);
         }

         QPixmapCache::insert (Key, pixmap);
      }

      _asset.previewLabel->setPixmap (pixmap);

      _asset.previewInfoLabel->setText (
         QString ("%1/%2").arg (_state.currentPreviewIndex)
                          .arg (_state.previews.get_count ()));
   }
}


dmz::Boolean
dmz::ForgePluginPublish::_dump_model (const String &File) {

   Boolean retVal (False);

   if (_state.core) {

      osg::Group *group = _state.core->lookup_dynamic_object (_state.target);
      if (group) {

         osg::Node *node = group->getChild (0);
         if (node) { retVal =  osgDB::writeNodeFile (*node, File.get_buffer ()); }
      }
   }

   return retVal;
}


void
dmz::ForgePluginPublish::_init (Config &local) {

   setObjectName (get_plugin_name ().get_buffer ());

   qframe_config_read ("frame", local, this);

   _state.forgeModuleName = config_to_string ("module.forge.name", local);

   RuntimeContext *context (get_plugin_runtime_context ());

   _state.attachMsg = config_create_message (
      "attach-message.name",
      local,
      "DMZ_Entity_Attach_Message",
      context);

   subscribe_to_message (_state.attachMsg);

   _state.modelAttrHandle = config_to_named_handle (
      "attribute.model.name",
      local,
      "Object_Model_Attribute",
      context);

   _state.startCaptureMsg = config_create_message (
      "start-screen-capture-message.name",
      local,
      "Start_Screen_Capture_Message",
      context);

   _state.finishedCaptureMsg = config_create_message (
      "finished-screen-capture-message.name",
      local,
      "Finished_Screen_Capture_Message",
      context);

    subscribe_to_message (_state.finishedCaptureMsg);

    _state.cleanupMsg = config_create_message (
       "cleanup-message.name",
       local,
       "CleanupObjectsMessage",
       context);

    _state.loadAssetMsg = config_create_message (
       "load-asset-message.name",
       local,
       "Load_Asset_Message",
       context);

    subscribe_to_message (_state.loadAssetMsg);
}


extern "C" {

DMZ_PLUGIN_FACTORY_LINK_SYMBOL dmz::Plugin *
create_dmzForgePluginPublish (
      const dmz::PluginInfo &Info,
      dmz::Config &local,
      dmz::Config &global) {

   return new dmz::ForgePluginPublish (Info, local);
}

};
