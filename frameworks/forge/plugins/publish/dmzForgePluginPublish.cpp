#include <dmzForgeConsts.h>
#include <dmzForgeModule.h>
#include "dmzForgePluginPublish.h"
#include <dmzObjectModule.h>
#include <dmzQtConfigRead.h>
#include <dmzRuntimeConfig.h>
#include <dmzRuntimeConfigToNamedHandle.h>
#include <dmzRuntimeConfigToTypesBase.h>
#include <dmzRuntimeDataConverterTypesBase.h>
#include <dmzRuntimeLog.h>
#include <dmzRenderModuleCoreOSG.h>
#include <dmzRuntimePluginFactoryLinkSymbol.h>
#include <dmzRuntimePluginInfo.h>
#include <dmzTypesStringContainer.h>
#include <QtGui/QtGui>
#include <osgDB/WriteFile>


struct dmz::ForgePluginPublish::State {

   Log log;
   DataConverterHandle convert;
   ObjectModule *objectModule;
   ForgeModule *forgeModule;
   RenderModuleCoreOSG *core;
   String forgeModuleName;
   Handle modelAttrHandle;
   String assetId;
   UInt64 requestId;
   Handle target;
   String mediaFile;

   State (const PluginInfo &Info);
};


dmz::ForgePluginPublish::State::State (const PluginInfo &Info) :
      log (Info),
      convert (Info),
      objectModule (0),
      forgeModule (0),
      forgeModuleName (),
      core (0),
      modelAttrHandle (0),
      assetId (),
      requestId (),
      target (0),
      mediaFile () {;}


dmz::ForgePluginPublish::ForgePluginPublish (const PluginInfo &Info, Config &local) :
      QFrame (0),
      QtWidget (Info),
      Plugin (Info),
      TimeSlice (Info, TimeSliceTypeSystemTime, TimeSliceModeSingle, 1.0),
      MessageObserver (Info),
      ForgeObserver (Info),
      _state (*(new State (Info))) {

   _ui.setupUi (this);

   _init (local);
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

      _ui.publishButton->setEnabled (False);
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
      }

      if (!_state.core) { _state.core = RenderModuleCoreOSG::cast (PluginPtr); }
   }
   else if (Mode == PluginDiscoverRemove) {

      if (_state.objectModule && (_state.objectModule == ObjectModule::cast (PluginPtr))) {

         _state.objectModule = 0;
      }

      if (_state.forgeModule && (_state.forgeModule == ForgeModule::cast (PluginPtr))) {

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

   _state.target = _state.convert.to_handle (InData);

   if (_state.target && _state.objectModule) {

      if (_state.objectModule->lookup_text (
            _state.target, _state.modelAttrHandle, _state.mediaFile)) {

         _ui.publishButton->setEnabled (True);
      }
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

      case ForgeTypeGetAsset:
         break;

      case ForgeTypePutAsset:
         if (RequestId == _state.requestId) { _publish_phase_2 (); }
         break;

      case ForgeTypeDeleteAsset:
         break;

      case ForgeTypeGetAssetMedia:
         break;

      case ForgeTypePutAssetMedia:
         if (RequestId == _state.requestId) { _publish_phase_3 (); }
         break;

      case ForgeTypeAddAssetPreview:
         if (RequestId == _state.requestId) { _publish_phase_4 (); }
         break;

      default:
         break;
   }

   if (Error) {

_state.log.error << "Results: " << Results << endl;
   }
}


void
dmz::ForgePluginPublish::on_publishButton_clicked () {

   _publish_phase_1 ();
}


void
dmz::ForgePluginPublish::_publish_phase_1 () {

   if (!_state.assetId && !_state.requestId && _state.forgeModule) {

      String name (qPrintable (_ui.nameLineEdit->text ()));
      String brief (qPrintable (_ui.briefLineEdit->text ()));
      String details (qPrintable (_ui.detailsTextEdit->toPlainText ()));

      _state.assetId = _state.forgeModule->create_asset ();

      _state.forgeModule->store_name (_state.assetId, name);
      _state.forgeModule->store_brief (_state.assetId, brief);
      _state.forgeModule->store_details (_state.assetId, details);

      _state.requestId = _state.forgeModule->put_asset (_state.assetId, this);

_state.log.warn << "AssetID: " << _state.assetId << " : " << _state.requestId << endl;
   }
}


void
dmz::ForgePluginPublish::_publish_phase_2 () {

   if (_state.requestId && _state.forgeModule && _state.objectModule) {

      QFileInfo fi (_state.mediaFile.get_buffer ());

      if (fi.suffix () == "dae") {

         //_state.rawFile = _state.mediaFile;
//         _state.mediaFile =
         _dump_model ();
      }

//      _state.requestId = _state.forgeModule->put_asset_media (_state.assetId, file, this);
   }
}


void
dmz::ForgePluginPublish::_publish_phase_3 () {

   if (_state.assetId && _state.forgeModule) {

   }
}


void
dmz::ForgePluginPublish::_publish_phase_4 () {

   if (_state.assetId && _state.forgeModule) {

   }
}


void
dmz::ForgePluginPublish::_dump_model () {

   if (_state.core) {

      osg::Group *group = _state.core->lookup_dynamic_object (_state.target);

      if (group) {

         osg::Node *node = group->getChild (0);

         if (node) {

            osgDB::ReaderWriter::WriteResult result =
               osgDB::Registry::instance()->writeNode (
                  *node,
                  "recognizer.ive",
                  osgDB::Registry::instance()->getOptions());

//            if (result.success())
//            {
//                osg::notify(osg::NOTICE)<<"Data written to '"<<fileNameOut<<"'."<< std::endl;
//            }
//            else if  (result.message().empty())
//            {
//                osg::notify(osg::NOTICE)<<"Warning: file write to '"<<fileNameOut<<"' no supported."<< std::endl;
//            }
//            else
//            {
//                osg::notify(osg::NOTICE)<<result.message()<< std::endl;
//            }

//            osgDB::writeNodeFile (*node, "recognizer.ive");
         }
      }
   }
}


void
dmz::ForgePluginPublish::_init (Config &local) {

   setObjectName (get_plugin_name ().get_buffer ());

   qframe_config_read ("frame", local, this);

   _state.forgeModuleName = config_to_string ("module.forge.name", local);

   Message msg = config_create_message (
      "attach-message.name",
      local,
      "DMZ_Entity_Attach_Message",
      get_plugin_runtime_context ());

   subscribe_to_message (msg);

   _state.modelAttrHandle = config_to_named_handle (
      "attribute.model.name",
      local,
      "Object_Model_Attribute",
      get_plugin_runtime_context ());
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
