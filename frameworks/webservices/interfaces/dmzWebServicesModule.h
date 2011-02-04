#ifndef DMZ_WEBSERVICES_MODULE_DOT_H
#define DMZ_WEBSERVICES_MODULE_DOT_H

#include <dmzWebServicesConsts.h>
#include <dmzRuntimePlugin.h>
#include <dmzRuntimeRTTI.h>
#include <dmzTypesBase.h>
#include <dmzTypesString.h>

namespace dmz {

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
          virtual Boolean register_webservices_observer (
            WebServicesObserver &observer) = 0;

          virtual Boolean release_webservices_observer (
            WebServicesObserver &observer) = 0;

         // virtual UInt64 login (
         //    const String &UserName,
         //    const String &Password
         //    WebServicesObserver *observer) = 0;
         //
         // virtual UInt64 logout (WebServicesObserver *observer) = 0;

         virtual Boolean is_recording () const = 0;

         virtual Handle start_session (const String &Name) = 0;

         virtual Boolean store_record (
            const Message &Type,
            const Handle Target,
            const Data *Record) = 0;

         virtual Boolean store_record (const Message &Type, const Data *Record) {

            return store_record (Type, 0, Record);
         }

         virtual Boolean stop_session (const Handle SessionHandle) = 0;
         virtual Boolean abort_session (const Handle SessionHandle) = 0;

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
