#ifndef DMZ_WEBSERVICES_MODULE_DOT_H
#define DMZ_WEBSERVICES_MODULE_DOT_H

#include <dmzRuntimePlugin.h>
#include <dmzRuntimeRTTI.h>
#include <dmzTypesBase.h>
#include <dmzTypesString.h>
#include <dmzTypesStringContainer.h>


namespace dmz {

   class Data;
   class Message;
   class WebServicesCallback;

   class WebServicesModule {

      public:
         static WebServicesModule *cast (
            const Plugin *PluginPtr,
            const String &PluginName = "");

         String get_webservices_module_name () const;
         Handle get_webservices_module_handle () const;

         // WebServicesModule Interface
         virtual Boolean is_valid_database (const Handle Database) = 0;
         virtual String lookup_database_name_from_handle (const Handle Databse) = 0;

         virtual Boolean publish_config (
            const Handle Database,
            const String &Id,
            const Config &Data,
            WebServicesCallback &cb) = 0;

         virtual Boolean fetch_config (
            const Handle Database,
            const String &Id,
            WebServicesCallback &cb) = 0;

         virtual Boolean fetch_configs (
            const Handle Database,
            const StringContainer &List,
            WebServicesCallback &cb) = 0;

         virtual Boolean delete_config (
            const Handle Database,
            const String &Id,
            WebServicesCallback &cb) = 0;

         virtual Boolean delete_configs (
            const Handle Database,
            const StringContainer &List,
            WebServicesCallback &cb) = 0;

         virtual Boolean fetch_updates (
            const Handle Database,
            WebServicesCallback &cb,
            const Int32 Since) = 0;

         virtual Boolean start_realtime_updates (
            const Handle Database,
            WebServicesCallback &cb,
            const Int32 Since = 0) = 0;

         virtual Boolean stop_realtime_updates (
            const Handle Database,
            WebServicesCallback &cb) = 0;

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
