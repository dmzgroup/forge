#ifndef DMZ_WEBSERVICES_CALLBACK_DOT_H
#define DMZ_WEBSERVICES_CALLBACK_DOT_H

#include <dmzRuntimePlugin.h>
#include <dmzRuntimePluginInfo.h>
#include <dmzRuntimeRTTI.h>
#include <dmzTypesBase.h>
#include <dmzTypesString.h>
#include <dmzTypesStringContainer.h>


namespace dmz {

   //! \cond
   const char WebServicesCallbackInterfaceName[] = "WebServicesCallbackInteface";
   //! \endcond

   class WebServicesCallback {

      public:
         static WebServicesCallback *cast (
            const Plugin *PluginPtr,
            const String &PluginName = "");

         static Boolean is_valid (const Handle ObserverHandle, RuntimeContext *context);

         Handle get_webservices_callback_handle ();
         String get_webservices_callback_name ();

         // WebServicesCallback Interface
         virtual void handle_error (
            const Handle Database,
            const String &Id,
            const Config &Data) = 0;

         virtual void handle_publish_config (
            const Handle Database,
            const String &Id,
            const String &Rev) = 0;

         virtual void handle_fetch_config (
            const Handle Database,
            const String &Id,
            const String &Rev,
            const Config &Data) = 0;

         virtual void handle_delete_config (
            const Handle Database,
            const String &Id,
            const String &Rev) = 0;

         virtual void handle_fetch_updates (
            const Handle Database,
            const Config &Updates) = 0;

         virtual void handle_realtime_update (
            const Handle Database,
            const String &Id,
            const String &Rev,
            const Boolean &Deleted,
            const Int32 Sequence) = 0;

      protected:
         WebServicesCallback (const PluginInfo &Info);
         ~WebServicesCallback ();

      private:
         WebServicesCallback ();
         WebServicesCallback (const WebServicesCallback &);
         WebServicesCallback &operator= (const WebServicesCallback &);

         const PluginInfo &__Info;
   };
};


inline dmz::WebServicesCallback *
dmz::WebServicesCallback::cast (const Plugin *PluginPtr, const String &PluginName) {

   return (WebServicesCallback *)lookup_rtti_interface (
      WebServicesCallbackInterfaceName,
      PluginName,
      PluginPtr);
}


inline dmz::Boolean
dmz::WebServicesCallback::is_valid (const Handle ObserverHandle, RuntimeContext *context) {

   return lookup_rtti_interface (
      WebServicesCallbackInterfaceName,
      ObserverHandle,
      context) != 0;
}


inline
dmz::WebServicesCallback::WebServicesCallback (const PluginInfo &Info) :
      __Info (Info) {

   store_rtti_interface (WebServicesCallbackInterfaceName, __Info, (void *)this);
}


inline
dmz::WebServicesCallback::~WebServicesCallback () {

   remove_rtti_interface (WebServicesCallbackInterfaceName, __Info);
}


inline dmz::String
dmz::WebServicesCallback::get_webservices_callback_name () { return __Info.get_name (); }


inline dmz::Handle
dmz::WebServicesCallback::get_webservices_callback_handle () { return __Info.get_handle (); }


#endif // DMZ_WEBSERVICES_CALLBACK_DOT_H

