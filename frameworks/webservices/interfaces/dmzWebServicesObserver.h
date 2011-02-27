#ifndef DMZ_WEBSERVICES_OBSERVER_DOT_H
#define DMZ_WEBSERVICES_OBSERVER_DOT_H

#include <dmzRuntimePlugin.h>
#include <dmzRuntimePluginInfo.h>
#include <dmzRuntimeRTTI.h>
#include <dmzTypesBase.h>
#include <dmzTypesString.h>
#include <dmzTypesStringContainer.h>


namespace dmz {

   //! \cond
   const char WebServicesObserverInterfaceName[] = "WebServicesObserverInteface";
   //! \endcond

   class WebServicesObserver {

      public:
         static WebServicesObserver *cast (
            const Plugin *PluginPtr,
            const String &PluginName = "");

         static Boolean is_valid (const Handle ObserverHandle, RuntimeContext *context);

         Handle get_webservices_observer_handle ();
         String get_webservices_observer_name ();

         // WebServicesObserver Interface
         virtual void handle_error (const String &Id, const Config &Data) = 0;

         virtual void handle_publish_config (
            const String &Id,
            const String &Rev) = 0;

         virtual void handle_fetch_config (
            const String &Id,
            const String &Rev,
            const Config &Data) = 0;

         virtual void handle_delete_config (
            const String &Id,
            const String &Rev) = 0;

         virtual void handle_fetch_updates (const Config &Updates) = 0;

         virtual void handle_realtime_update (
            const String &Id,
            const String &Rev,
            const Boolean &Deleted,
            const Int32 Sequence) = 0;

      protected:
         WebServicesObserver (const PluginInfo &Info);
         ~WebServicesObserver ();

      private:
         WebServicesObserver ();
         WebServicesObserver (const WebServicesObserver &);
         WebServicesObserver &operator= (const WebServicesObserver &);

         const PluginInfo &__Info;
   };
};


inline dmz::WebServicesObserver *
dmz::WebServicesObserver::cast (const Plugin *PluginPtr, const String &PluginName) {

   return (WebServicesObserver *)lookup_rtti_interface (
      WebServicesObserverInterfaceName,
      PluginName,
      PluginPtr);
}


inline dmz::Boolean
dmz::WebServicesObserver::is_valid (const Handle ObserverHandle, RuntimeContext *context) {

   return lookup_rtti_interface (
      WebServicesObserverInterfaceName,
      ObserverHandle,
      context) != 0;
}


inline
dmz::WebServicesObserver::WebServicesObserver (const PluginInfo &Info) :
      __Info (Info) {

   store_rtti_interface (WebServicesObserverInterfaceName, __Info, (void *)this);
}


inline
dmz::WebServicesObserver::~WebServicesObserver () {

   remove_rtti_interface (WebServicesObserverInterfaceName, __Info);
}


inline dmz::String
dmz::WebServicesObserver::get_webservices_observer_name () { return __Info.get_name (); }


inline dmz::Handle
dmz::WebServicesObserver::get_webservices_observer_handle () { return __Info.get_handle (); }


#endif // DMZ_WEBSERVICES_OBSERVER_DOT_H

