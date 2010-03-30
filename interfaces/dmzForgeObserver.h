#ifndef DMZ_FORGE_OBSERVER_DOT_H
#define DMZ_FORGE_OBSERVER_DOT_H

#include <dmzForgeConsts.h>
#include <dmzRuntimePlugin.h>
#include <dmzRuntimePluginInfo.h>
#include <dmzRuntimeRTTI.h>
#include <dmzTypesBase.h>
#include <dmzTypesString.h>

namespace dmz {

   //! \cond
   const char ForgeObserverInterfaceName[] = "ForgeObserverInteface";
   //! \endcond

   class ForgeObserver {

      public:
         static ForgeObserver *cast (
            const Plugin *PluginPtr,
            const String &PluginName = "");

         static Boolean is_valid (const Handle ObserverHandle, RuntimeContext *context);

         Handle get_forge_observer_handle ();
         String get_forge_observer_name ();
         
         virtual void handle_reply (
            const UInt64 RequestId,
            const ForgeRequestTypeEnum RequestType,
            const Boolean Error,
            const StringContainer &Results) = 0;

      protected:
         ForgeObserver (const PluginInfo &Info);
         ~ForgeObserver ();

      private:
         ForgeObserver ();
         ForgeObserver (const ForgeObserver &);
         ForgeObserver &operator= (const ForgeObserver &);

         const PluginInfo &__Info;
   };
};


inline dmz::ForgeObserver *
dmz::ForgeObserver::cast (const Plugin *PluginPtr, const String &PluginName) {

   return (ForgeObserver *)lookup_rtti_interface (
      ForgeObserverInterfaceName,
      PluginName,
      PluginPtr);
}


inline dmz::Boolean
dmz::ForgeObserver::is_valid (const Handle ObserverHandle, RuntimeContext *context) {

   return lookup_rtti_interface (
      ForgeObserverInterfaceName,
      ObserverHandle,
      context) != 0;
}


inline
dmz::ForgeObserver::ForgeObserver (const PluginInfo &Info) :
      __Info (Info) {

   store_rtti_interface (ForgeObserverInterfaceName, __Info, (void *)this);
}


inline
dmz::ForgeObserver::~ForgeObserver () {

   remove_rtti_interface (ForgeObserverInterfaceName, __Info);
}


inline dmz::String
dmz::ForgeObserver::get_forge_observer_name () { return __Info.get_name (); }


inline dmz::Handle
dmz::ForgeObserver::get_forge_observer_handle () { return __Info.get_handle (); }


#endif // DMZ_FORGE_OBSERVER_DOT_H

