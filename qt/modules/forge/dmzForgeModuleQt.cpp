#include "dmzForgeModuleQt.h"
#include <dmzForgeObserver.h>
#include "dmzForgeWebServiceQt.h"
#include <dmzFoundationJSONUtil.h>
#include <dmzRuntimeConfig.h>
#include <dmzRuntimeConfigToTypesBase.h>
#include <dmzRuntimePluginFactoryLinkSymbol.h>
#include <dmzRuntimePluginInfo.h>
#include <dmzRuntimeLog.h>
#include <dmzTypesHashTableStringTemplate.h>
#include <dmzTypesHashTableUInt64Template.h>
#include <dmzTypesString.h>
#include <dmzTypesStringContainer.h>
#include <QtCore/QtCore>
#include <QtNetwork/QtNetwork>


namespace {

   static const dmz::String JsonParseErrorMessage ("Error parsing json response.");
};


struct dmz::ForgeModuleQt::AssetStruct {

   const String Id;
   ForgeAssetTypeEnum type;
   String name;
   String brief;
   String details;
   StringContainer keywords;
   StringContainer media;
   StringContainer previews;
   
   AssetStruct (const String AssetId) : Id (AssetId) {;}
};


struct dmz::ForgeModuleQt::State {

   Log log;
   ForgeWebServiceQt *ws;
   StringContainer uuids;
   HashTableUInt64Template<ForgeObserver> obsTable;
   HashTableStringTemplate<AssetStruct> assetTable;
   
   State (const PluginInfo &Info);
   ~State ();
};


dmz::ForgeModuleQt::State::State (const PluginInfo &Info) :
      log (Info),
      ws (0) {;}


dmz::ForgeModuleQt::State::~State () {

   obsTable.clear ();
   assetTable.empty ();
}


dmz::ForgeModuleQt::ForgeModuleQt (const PluginInfo &Info, Config &local) :
      QObject (0),
      Plugin (Info),
      ForgeModule (Info),
      _state (*(new State (Info))) {

   _state.ws = new ForgeWebServiceQt (&_state.log, this);
   
   connect (
      _state.ws, SIGNAL (finished (QNetworkReply *)),
      this, SLOT (_handle_reply (QNetworkReply *)));
   
   _init (local);
}


dmz::ForgeModuleQt::~ForgeModuleQt () {

   delete &_state;
}


// Plugin Interface
void
dmz::ForgeModuleQt::update_plugin_state (
      const PluginStateEnum State,
      const UInt32 Level) {

   if (State == PluginStateInit) {

      if (_state.ws) {
         
         _state.ws->get_uuids (100);
      }
   }
   else if (State == PluginStateStart) {

   }
   else if (State == PluginStateStop) {

   }
   else if (State == PluginStateShutdown) {

   }
}


void
dmz::ForgeModuleQt::discover_plugin (
      const PluginDiscoverEnum Mode,
      const Plugin *PluginPtr) {

   if (Mode == PluginDiscoverAdd) {

   }
   else if (Mode == PluginDiscoverRemove) {

   }
}


// ForgeModule Interface

dmz::Boolean
dmz::ForgeModuleQt::is_saved (const String &AssetId) {

   Boolean retVal (False);
   
   return retVal;
}


dmz::String
dmz::ForgeModuleQt::create_asset (const String &AssetId) {

}


dmz::ForgeAssetTypeEnum
dmz::ForgeModuleQt::lookup_asset_type (const String &AssetId) {

   ForgeAssetTypeEnum retVal (ForgeAssetUnknown);

   AssetStruct *asset = _state.assetTable.lookup (AssetId);
   if (asset) { retVal = asset->type; }

   return retVal;
}


dmz::Boolean
dmz::ForgeModuleQt::store_name (const String &AssetId, const String &Value) {
   
   Boolean retVal (False);
   
   AssetStruct *asset = _state.assetTable.lookup (AssetId);
   if (asset) {
      
      asset->name = Value;
      retVal = True;
   }
   
   return retVal;
}


dmz::Boolean
dmz::ForgeModuleQt::lookup_name (const String &AssetId, String &value) {
   
   Boolean retVal (False);

   AssetStruct *asset = _state.assetTable.lookup (AssetId);
   if (asset) {
      
      value = asset->name;
      retVal = True;
   }
   
   return retVal;
}


dmz::Boolean
dmz::ForgeModuleQt::store_brief (const String &AssetId, const String &Value) {
   
   Boolean retVal (False);

   AssetStruct *asset = _state.assetTable.lookup (AssetId);
   if (asset) {
      
      asset->brief = Value;
      retVal = True;
   }
   
   return retVal;
}


dmz::Boolean
dmz::ForgeModuleQt::lookup_brief (const String &AssetId, String &value) {
   
   Boolean retVal (False);
   
   AssetStruct *asset = _state.assetTable.lookup (AssetId);
   if (asset) {
      
      value = asset->brief;
      retVal = True;
   }
   
   return retVal;
}


dmz::Boolean
dmz::ForgeModuleQt::store_details (const String &AssetId, const String &Value) {
   
   Boolean retVal (False);
   
   AssetStruct *asset = _state.assetTable.lookup (AssetId);
   if (asset) {
      
      asset->details = Value;
      retVal = True;
   }
   
   return retVal;
}


dmz::Boolean
dmz::ForgeModuleQt::lookup_details (const String &AssetId, String &value) {
   
   Boolean retVal (False);
   
   AssetStruct *asset = _state.assetTable.lookup (AssetId);
   if (asset) {
      
      value = asset->details;
      retVal = True;
   }
   
   return retVal;
}


dmz::Boolean
dmz::ForgeModuleQt::store_keywords (const String &AssetId, const StringContainer &Value) {

   Boolean retVal (False);
   
   AssetStruct *asset = _state.assetTable.lookup (AssetId);
   if (asset) {
      
      asset->keywords = Value;
      retVal = True;
   }
   
   return retVal;
}


dmz::Boolean
dmz::ForgeModuleQt::lookup_keywords (const String &AssetId, StringContainer &value) {
   
   Boolean retVal (False);
   
   AssetStruct *asset = _state.assetTable.lookup (AssetId);
   if (asset) {
      
      value = asset->keywords;
      retVal = True;
   }
   
   return retVal;
}


dmz::Boolean
dmz::ForgeModuleQt::lookup_asset_media (const String &AssetId, StringContainer &value) {
   
   Boolean retVal (False);
   
   AssetStruct *asset = _state.assetTable.lookup (AssetId);
   if (asset) {
      
      value = asset->media;
      retVal = True;
   }
   
   return retVal;
}


dmz::Boolean
dmz::ForgeModuleQt::lookup_previews (const String &AssetId, StringContainer &value) {
      
   Boolean retVal (False);
   
   AssetStruct *asset = _state.assetTable.lookup (AssetId);
   if (asset) {
      
      value = asset->previews;
      retVal = True;
   }
   
   return retVal;
}


// methods that communicate with api.dmzforge.org

dmz::UInt64
dmz::ForgeModuleQt::search (
      const String &Value,
      ForgeObserver *observer,
      const UInt32 Limit) {

   UInt64 requestId (0);
   
   if (_state.ws && observer) {
      
      QNetworkReply *reply = _state.ws->search (Value, Limit);
      if (reply) {
         
         requestId = _get_request_id (reply);
         _state.obsTable.store (requestId, observer);
      }
   }
   
   return requestId;
}


dmz::UInt64
dmz::ForgeModuleQt::get_asset (const String &AssetId, ForgeObserver *observer) {

   UInt64 requestId (0);
   
   if (_state.ws && observer) {
      
      QNetworkReply *reply = _state.ws->get_asset (AssetId);
      if (reply) {
         
         requestId = _get_request_id (reply);
         _state.obsTable.store (requestId, observer);
      }
   }
   
   return requestId;
}


dmz::UInt64
dmz::ForgeModuleQt::put_asset (const String &AssetId, ForgeObserver *observer) {

   UInt64 retVal (0);
   
   return retVal;
}


dmz::UInt64
dmz::ForgeModuleQt::delete_asset (const String &AssetId, ForgeObserver *observer) {

   UInt64 retVal (0);
   
   return retVal;
}


dmz::UInt64
dmz::ForgeModuleQt::get_asset_media (
      const String &AssetId,
      const String &File,
      ForgeObserver *observer) {

   UInt64 retVal (0);

   return retVal;
}


dmz::UInt64
dmz::ForgeModuleQt::put_asset_media (
      const String &AssetId,
      const String &File,
      ForgeObserver *observer,
      const String &MimeType) {

   UInt64 retVal (0);

   return retVal;
}


dmz::UInt64
dmz::ForgeModuleQt::get_preview (
      const String &AssetId,
      const String &File,
      ForgeObserver *observer) {

         UInt64 retVal (0);

         return retVal;
}


dmz::UInt64
dmz::ForgeModuleQt::put_preview (
      const String &AssetId,
      const String &Preview,
      ForgeObserver *observer) {

   UInt64 retVal (0);

   return retVal;
}


dmz::UInt64
dmz::ForgeModuleQt::_get_request_id (QNetworkReply *reply) {

   UInt64 requestId (0);
   if (reply) { requestId = reply->property ("requestId").toULongLong (); }
   return requestId;
}


void
dmz::ForgeModuleQt::_handle_reply (QNetworkReply *reply) {
   
   if (reply) {

      const QString RequestType (reply->property ("requestType").toString ());
      const UInt64 RequestId (_get_request_id (reply));
      
qDebug () << "_handle_reply: " << reply->property ("requestType").toString ();
      
      if (reply->error() == QNetworkReply::NoError) {

         QString data (reply->readAll ());
         const String JsonData (qPrintable (data));
         
         if (RequestType == ForgeSearchName) {
            
            _handle_search (RequestId, JsonData);
         }
         else if (RequestType == "get_uuids") {
            
            _handle_get_uuids (RequestId, JsonData);
         }
         else if (RequestType == ForgeGetAssetName) {
            
            _handle_get_asset (RequestId, JsonData);
         }
         else {
            
            String msg ("Unknown request type: ");
            msg << qPrintable (RequestType);

            _handle_error (RequestId, qPrintable (RequestType), msg);
         }
      }
      else {
         
         String msg ("Network Error: ");
         msg << qPrintable (reply->errorString ());
         
         _handle_error (RequestId, qPrintable (RequestType), msg);
      }
      
      _state.obsTable.remove (RequestId);
   }
}


void
dmz::ForgeModuleQt::_handle_search (const UInt64 RequestId, const String &JsonData) {

   Config global ("global");
   
   if (json_string_to_config (JsonData, global)) { 
   
      StringContainer container;
      Config list;

      if (global.lookup_all_config ("rows", list)) {

         ConfigIterator it;
         Config cd;

         while (list.get_next_config (it, cd)) {

            String assetId = config_to_string ("id", cd);
            if (assetId) { container.append (assetId); }
         }
      }
      
      _handle_reply (RequestId, ForgeSearchName, container);
   }
   else {
      
      _handle_error (RequestId, ForgeSearchName, JsonParseErrorMessage);
   }
}


void
dmz::ForgeModuleQt::_handle_get_uuids (const UInt64 RequestId, const String &JsonData) {

   Config global ("global");
   
   if (json_string_to_config (JsonData, global)) { 

      _state.uuids.clear ();
      Config list;
   
      if (global.lookup_all_config ("uuids", list)) {
   
         ConfigIterator it;
         Config cd;
   
         while (list.get_next_config (it, cd)) {
   
            String id = config_to_string ("value", cd);
            if (id) { _state.uuids.append (id); }
         }
         
_state.log.error << "uuids: " << _state.uuids << endl;
      }
   }
}


void
dmz::ForgeModuleQt::_handle_get_asset (const UInt64 RequestId, const String &JsonData) {

   Config global ("global");
   
   if (json_string_to_config (JsonData, global)) { 
   
      _handle_asset (global);
      
      StringContainer container;
      container.append (JsonData);
      
      _handle_reply (RequestId, ForgeGetAssetName, container);
   }
   else {
      
      _handle_error (RequestId, ForgeGetAssetName, JsonParseErrorMessage);
   }
}


void
dmz::ForgeModuleQt::_handle_reply (
      const UInt64 RequestId,
      const String &RequestType,
      StringContainer &Container) {

   ForgeObserver *observer = _state.obsTable.lookup (RequestId);
   if (observer) {
      
      observer->handle_reply (RequestId, RequestType, False, Container);
   }
}


void
dmz::ForgeModuleQt::_handle_error (
      const UInt64 RequestId,
      const String &RequestType,
      const String &Message) {

   ForgeObserver *observer = _state.obsTable.lookup (RequestId);
   if (observer) {
      
      StringContainer container;
      container.append (Message);
      
      observer->handle_reply (RequestId, RequestType, True, container);
   }
}


void
dmz::ForgeModuleQt::_handle_asset (const Config &Data) {
   
}


void
dmz::ForgeModuleQt::_init (Config &local) {

   setObjectName (get_plugin_name ().get_buffer ());
}


extern "C" {

DMZ_PLUGIN_FACTORY_LINK_SYMBOL dmz::Plugin *
create_dmzForgeModuleQt (
      const dmz::PluginInfo &Info,
      dmz::Config &local,
      dmz::Config &global) {

   return new dmz::ForgeModuleQt (Info, local);
}

};
