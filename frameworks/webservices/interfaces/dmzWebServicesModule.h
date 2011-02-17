#ifndef DMZ_WEBSERVICES_MODULE_DOT_H
#define DMZ_WEBSERVICES_MODULE_DOT_H

#include <dmzWebServicesConsts.h>
#include <dmzRuntimePlugin.h>
#include <dmzRuntimeRTTI.h>
#include <dmzTypesBase.h>
#include <dmzTypesString.h>
#include <dmzTypesStringContainer.h>


namespace dmz {

   class Data;
   class Message;
   class WebServicesObserver;

   class WebServicesModule {

      public:
         static WebServicesModule *cast (
            const Plugin *PluginPtr,
            const String &PluginName = "");

         String get_webservices_module_name () const;
         Handle get_webservices_module_handle () const;

         // WebServicesModule Interface
         virtual Boolean publish_config (
            const String &Id,
            const Config &Data,
            WebServicesObserver &obs) = 0;

         virtual Boolean fetch_config (const String &Id, WebServicesObserver &obs) = 0;

         virtual Boolean fetch_configs (
            const StringContainer &IdList,
            WebServicesObserver &obs) = 0;

         virtual Boolean delete_config (const String &Id, WebServicesObserver &obs) = 0;

         virtual Boolean delete_configs (
            const StringContainer &IdList,
            WebServicesObserver &obs) = 0;

         virtual Boolean get_config_updates (
            WebServicesObserver &obs,
            const Int32 Since,
            const Boolean Heavy = False) = 0;

         virtual Boolean start_realtime_updates (
            WebServicesObserver &obs,
            const Int32 Since = 0) = 0;

//         virtual Boolean stop_realtime_updates (WebServicesObserver &obs) = 0;

      protected:
         WebServicesModule (const PluginInfo &Info);
         ~WebServicesModule ();

      private:
         WebServicesModule ();
         WebServicesModule (const WebServicesModule &);
         WebServicesModule &operator= (const WebServicesModule &);

         const PluginInfo &__Info;
   };

   //! \cond
   const char WebServicesModuleInterfaceName[] = "WebServicesModuleInterface";
   //! \endcond
};


inline dmz::WebServicesModule *
dmz::WebServicesModule::cast (const Plugin *PluginPtr, const String &PluginName) {

   return (WebServicesModule *)lookup_rtti_interface (
      WebServicesModuleInterfaceName,
      PluginName,
      PluginPtr);
}


inline
dmz::WebServicesModule::WebServicesModule (const PluginInfo &Info) :
      __Info (Info) {

   store_rtti_interface (WebServicesModuleInterfaceName, __Info, (void *)this);
}


inline
dmz::WebServicesModule::~WebServicesModule () {

   remove_rtti_interface (WebServicesModuleInterfaceName, __Info);
}


inline dmz::String
dmz::WebServicesModule::get_webservices_module_name () const { return __Info.get_name (); }


inline dmz::Handle
dmz::WebServicesModule::get_webservices_module_handle () const { return __Info.get_handle (); }

#endif // DMZ_WEBSERVICES_MODULE_DOT_H
