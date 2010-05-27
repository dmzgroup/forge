/*!

\class dmz::ForgeObserver
\ingroup Forge
\brief Observers forge asyncronous 
\details A pure virtual interface for observing forge events. This class is only an interface.

\fn dmz::ForgeObserver::ForgeObserver (const PluginInfo &Info)
\brief Constructor
\param[in] Info PluginInfo containing initialization data.

\fn dmz::ForgeObserver::~ForgeObserver ()
\brief Destructor

\fn dmz::ForgeObserver::ForgeObserver *dmz::ForgeObserver::cast (
const Plugin *PluginPtr,
const String &PluginName)
\brief Casts Plugin pointer to a ForgeObserver pointer.
\details If the Plugin object implements the ForgeObserver interface, apointer to the
ForgeObserver interface of the Plugin is returned.
\param[in] PluginPtr Pointer to the Plugin.
\param[in] PluginName String containing name of desired ForgeObserver.
\return Returns a pointer to the ForgeObserver. Returns NULL if the Plugin does not
implement the ForgeObserver interface or the \a PluginName is not empty and not
equal to the Plugin's name.

\fn dmz::Boolean dmz::ForgeObserver::is_valid (
const Handle ObserverHandle,
RuntimeContext *context)
\brief Tests if runtime handle belongs to a valid forge observer interface.
\param[in] ObserverHandle Unique runtime handle.
\param[in] context Pointer to the runtime context.
\return Returns dmz::True if the runtime handle is valid and the associated Plugin
supports the forge observer interface.

\fn dmz::Handle dmz::ForgeObserver::get_forge_observer_handle ()
\brief Gets forge observers unique runtime handle.
\return Returns unique runtime handle.

\fn dmz::Handle dmz::ForgeObserver::get_forge_observer_name ()
\brief Gets forge observers name.
\return Returns forge observer name.

\fn void dmz::ForgeObserver::receive_results (
const UInt32 EventId,
const ForgeEventEnum EventType,
const String &AssetId,
const Boolean Succeeded,
const StringContainer &Results)
\brief Receives the results from an asyncronous forge method call.
\param[in] EventId Events unique id.
\param[in] EventType Event type enumeration.
\param[in] AssetId Assets unique id.
\param[in] Error True if an error was encountered.
\param[in] Results One or more strings returned from the server. If Error was true then 
results will conatian a string that describes the specific error.


*/
