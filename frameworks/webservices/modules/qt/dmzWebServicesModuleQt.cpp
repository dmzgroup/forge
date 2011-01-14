#include <dmzRuntimeConfig.h>
#include <dmzRuntimeConfigToTypesBase.h>
#include <dmzRuntimePluginFactoryLinkSymbol.h>
#include <dmzRuntimePluginInfo.h>
#include <dmzRuntimeLog.h>
#include <dmzTypesString.h>
#include <dmzTypesStringContainer.h>
#include <dmzTypesUUID.h>
#include <dmzFoundationJSONUtil.h>
#include "dmzWebServicesModuleQt.h"
#include <QtCore/QtCore>
#include <QtNetwork/QtNetwork>

using namespace dmz;


namespace {
   
   static const char LocalApiEndpoint[] = "http://api.dmzforge.org:80";
   static const char LocalUserAgentName[] = "dmzForgeModuleQt";
   
   static const String Local_Id ("_id");
   static const String Local_Rev ("_rev");
   static const String Local_Attachments ("_attachments");
   
   static const QString LocalGet ("get");
   static const QString LocalPut ("put");
   static const QString LocalDelete ("delete");
//   static const QString LocalTimeStampFormat ("yyyy-M-d-h-m-s");
};


struct dmz::WebServicesModuleQt::State {

   Log log;
   QNetworkAccessManager *networkAccessManager;
   QUrl baseUrl;
   // HashTableUInt64Template<ForgeObserver> obsTable;
   
   State (const PluginInfo &Info);
   ~State ();
};


dmz::WebServicesModuleQt::State::State (const PluginInfo &Info) :
      log (Info),
      networkAccessManager (0) {
   
}


dmz::WebServicesModuleQt::State::~State () {

   if (networkAccessManager) {

      networkAccessManager->deleteLater ();
      networkAccessManager = 0;
   }
}


dmz::WebServicesModuleQt::WebServicesModuleQt (const PluginInfo &Info, Config &local) :
      QObject (0),
      Plugin (Info),
      TimeSlice (Info),
      ObjectObserverUtil (Info, local),
      WebServicesModule (Info),
      _state (*(new State (Info))) {

   _state.networkAccessManager = new QNetworkAccessManager (this);

   stop_time_slice ();
   
   _init (local);
}


dmz::WebServicesModuleQt::~WebServicesModuleQt () {

   delete &_state;
}


// Plugin Interface
void
dmz::WebServicesModuleQt::update_plugin_state (
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
dmz::WebServicesModuleQt::discover_plugin (
      const PluginDiscoverEnum Mode,
      const Plugin *PluginPtr) {

   if (Mode == PluginDiscoverAdd) {

   }
   else if (Mode == PluginDiscoverRemove) {

   }
}


// TimeSlice Interface
void
dmz::WebServicesModuleQt::update_time_slice (const Float64 TimeDelta) {

   // while (!_state.replyQueue.isEmpty ()) {
   // 
   //    ReplyStruct *rs = _state.replyQueue.dequeue ();
   // 
   //    if (_state.replyQueue.isEmpty ()) { stop_time_slice (); }
   // 
   //    ForgeObserver *observer = _state.obsTable.remove (rs->RequestId);
   //    if (observer) {
   // 
   //       observer->handle_reply (
   //          rs->RequestId,
   //          rs->RequestType,
   //          rs->Error,
   //          rs->Container);
   //    }
   // 
   //    delete rs; rs = 0;
   // }
}


// Object Observer Interface
void
dmz::WebServicesModuleQt::create_object (
      const UUID &Identity,
      const Handle ObjectHandle,
      const ObjectType &Type,
      const ObjectLocalityEnum Locality) {

}


void
dmz::WebServicesModuleQt::destroy_object (
      const UUID &Identity,
      const Handle ObjectHandle) {

}


void
dmz::WebServicesModuleQt::update_object_uuid (
      const Handle ObjectHandle,
      const UUID &Identity,
      const UUID &PrevIdentity) {

}


void
dmz::WebServicesModuleQt::remove_object_attribute (
      const UUID &Identity,
      const Handle ObjectHandle,
      const Handle AttributeHandle,
      const Mask &AttrMask) {

}


void
dmz::WebServicesModuleQt::update_object_locality (
      const UUID &Identity,
      const Handle ObjectHandle,
      const ObjectLocalityEnum Locality,
      const ObjectLocalityEnum PrevLocality) {

}


void
dmz::WebServicesModuleQt::link_objects (
      const Handle LinkHandle,
      const Handle AttributeHandle,
      const UUID &SuperIdentity,
      const Handle SuperHandle,
      const UUID &SubIdentity,
      const Handle SubHandle) {

}


void
dmz::WebServicesModuleQt::unlink_objects (
      const Handle LinkHandle,
      const Handle AttributeHandle,
      const UUID &SuperIdentity,
      const Handle SuperHandle,
      const UUID &SubIdentity,
      const Handle SubHandle) {

}


void
dmz::WebServicesModuleQt::update_link_attribute_object (
      const Handle LinkHandle,
      const Handle AttributeHandle,
      const UUID &SuperIdentity,
      const Handle SuperHandle,
      const UUID &SubIdentity,
      const Handle SubHandle,
      const UUID &AttributeIdentity,
      const Handle AttributeObjectHandle,
      const UUID &PrevAttributeIdentity,
      const Handle PrevAttributeObjectHandle) {

}


void
dmz::WebServicesModuleQt::update_object_counter (
      const UUID &Identity,
      const Handle ObjectHandle,
      const Handle AttributeHandle,
      const Int64 Value,
      const Int64 *PreviousValue) {

}


void
dmz::WebServicesModuleQt::update_object_counter_minimum (
      const UUID &Identity,
      const Handle ObjectHandle,
      const Handle AttributeHandle,
      const Int64 Value,
      const Int64 *PreviousValue) {

}


void
dmz::WebServicesModuleQt::update_object_counter_maximum (
      const UUID &Identity,
      const Handle ObjectHandle,
      const Handle AttributeHandle,
      const Int64 Value,
      const Int64 *PreviousValue) {

}


void
dmz::WebServicesModuleQt::update_object_alternate_type (
      const UUID &Identity,
      const Handle ObjectHandle,
      const Handle AttributeHandle,
      const ObjectType &Value,
      const ObjectType *PreviousValue) {

}


void
dmz::WebServicesModuleQt::update_object_state (
      const UUID &Identity,
      const Handle ObjectHandle,
      const Handle AttributeHandle,
      const Mask &Value,
      const Mask *PreviousValue) {

}


void
dmz::WebServicesModuleQt::update_object_flag (
      const UUID &Identity,
      const Handle ObjectHandle,
      const Handle AttributeHandle,
      const Boolean Value,
      const Boolean *PreviousValue) {

}


void
dmz::WebServicesModuleQt::update_object_time_stamp (
      const UUID &Identity,
      const Handle ObjectHandle,
      const Handle AttributeHandle,
      const Float64 Value,
      const Float64 *PreviousValue) {

}


void
dmz::WebServicesModuleQt::update_object_position (
      const UUID &Identity,
      const Handle ObjectHandle,
      const Handle AttributeHandle,
      const Vector &Value,
      const Vector *PreviousValue) {

}


void
dmz::WebServicesModuleQt::update_object_orientation (
      const UUID &Identity,
      const Handle ObjectHandle,
      const Handle AttributeHandle,
      const Matrix &Value,
      const Matrix *PreviousValue) {

}


void
dmz::WebServicesModuleQt::update_object_velocity (
      const UUID &Identity,
      const Handle ObjectHandle,
      const Handle AttributeHandle,
      const Vector &Value,
      const Vector *PreviousValue) {

}


void
dmz::WebServicesModuleQt::update_object_acceleration (
      const UUID &Identity,
      const Handle ObjectHandle,
      const Handle AttributeHandle,
      const Vector &Value,
      const Vector *PreviousValue) {

}


void
dmz::WebServicesModuleQt::update_object_scale (
      const UUID &Identity,
      const Handle ObjectHandle,
      const Handle AttributeHandle,
      const Vector &Value,
      const Vector *PreviousValue) {

}


void
dmz::WebServicesModuleQt::update_object_vector (
      const UUID &Identity,
      const Handle ObjectHandle,
      const Handle AttributeHandle,
      const Vector &Value,
      const Vector *PreviousValue) {

}


void
dmz::WebServicesModuleQt::update_object_scalar (
      const UUID &Identity,
      const Handle ObjectHandle,
      const Handle AttributeHandle,
      const Float64 Value,
      const Float64 *PreviousValue) {

}


void
dmz::WebServicesModuleQt::update_object_text (
      const UUID &Identity,
      const Handle ObjectHandle,
      const Handle AttributeHandle,
      const String &Value,
      const String *PreviousValue) {

}


void
dmz::WebServicesModuleQt::update_object_data (
      const UUID &Identity,
      const Handle ObjectHandle,
      const Handle AttributeHandle,
      const Data &Value,
      const Data *PreviousValue) {

}


// WebServicesModuleQt Interface

void
dmz::WebServicesModuleQt::_init (Config &local) {

   setObjectName (get_plugin_name ().get_buffer ());
   
   Config webservice;
   if (local.lookup_config ("webservice", webservice)) {

      // Config proxy;
      // if (webservice.lookup_config ("proxy", proxy)) {
      // 
      //    String host = config_to_string ("host", proxy, "localhost");
      //    Int32 port = config_to_int32 ("port", proxy, 8888);
      // 
      //    QNetworkProxy proxy;
      //    proxy.setType (QNetworkProxy::HttpProxy);
      //    proxy.setHostName (host.get_buffer ());
      //    proxy.setPort (port);
      // 
      //    QNetworkProxy::setApplicationProxy(proxy);
      // 
      //    _state.log.info << "Using proxy: " << host << ":" << port << endl;
      // }
      // 
      // String host = qPrintable (_state.baseUrl.host ());
      // host =  config_to_string ("host", webservice, host);
      // _state.baseUrl.setHost (host.get_buffer ());
      // 
      // Int32 port = _state.baseUrl.port ();
      // port = config_to_int32 ("port", webservice, port);
      // _state.baseUrl.setPort (port);
      // 
      // _state.log.info
      //    << "Using API endpoint: " << qPrintable (_state.baseUrl.toString ()) << endl;
      // 
      // _state.db = config_to_string ("db", webservice, "assets").get_buffer ();
   }
}


extern "C" {

DMZ_PLUGIN_FACTORY_LINK_SYMBOL dmz::Plugin *
create_dmzWebServicesModuleQt (
      const dmz::PluginInfo &Info,
      dmz::Config &local,
      dmz::Config &global) {

   return new dmz::WebServicesModuleQt (Info, local);
}

};
