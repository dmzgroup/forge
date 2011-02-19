#include <dmzQtModuleMainWindow.h>
#include "dmzQtPluginLoginDialog.h"
#include <dmzRuntimeConfigToNamedHandle.h>
#include <dmzRuntimeConfigToTypesBase.h>
#include <dmzRuntimeData.h>
#include <dmzRuntimePluginFactoryLinkSymbol.h>
#include <dmzRuntimePluginInfo.h>
#include <QtGui/QtGui>


dmz::QtPluginLoginDialog::QtPluginLoginDialog (const PluginInfo &Info, Config &local) :
      QObject (0),
      Plugin (Info),
      MessageObserver (Info),
      _log (Info),
      _mainWindowModule (0),
      _mainWindowModuleName (),
      _usernameHandle (0),
      _passwordHandle (0),
      _targetHandle (0),
      _loggedIn (False),
      _loginDialog (0) {

   _init (local);
}


dmz::QtPluginLoginDialog::~QtPluginLoginDialog () {

   if (_loginDialog) {

      _loginDialog->setParent (0);
      delete _loginDialog;
      _loginDialog = 0;
   }
}


// Plugin Interface
void
dmz::QtPluginLoginDialog::update_plugin_state (
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
dmz::QtPluginLoginDialog::discover_plugin (
      const PluginDiscoverEnum Mode,
      const Plugin *PluginPtr) {

   if (Mode == PluginDiscoverAdd) {

      if (!_mainWindowModule) {

         _mainWindowModule = QtModuleMainWindow::cast (PluginPtr, _mainWindowModuleName);
         if (_mainWindowModule) {

            _loginDialog = new QDialog (_mainWindowModule->get_qt_main_window ());
            _ui.setupUi (_loginDialog);

            connect (
               _loginDialog, SIGNAL (accepted ()),
               this, SLOT (_slot_dialog_accepted ()));

            connect (
               _loginDialog, SIGNAL (rejected ()),
               this, SLOT (_slot_dialog_rejected ()));
         }
      }
   }
   else if (Mode == PluginDiscoverRemove) {

      if (_mainWindowModule &&
            (_mainWindowModule == QtModuleMainWindow::cast (PluginPtr))) {

         if (_loginDialog) { _loginDialog->setParent (0); }

         _mainWindowModule = 0;
      }
   }
}


// Message Observer Interface
void
dmz::QtPluginLoginDialog::receive_message (
      const Message &Type,
      const UInt32 MessageSendHandle,
      const Handle TargetObserverHandle,
      const Data *InData,
      Data *outData) {

   if (Type == _loginRequiredMsg) {

      if (_loginDialog) { _loginDialog->open (); }
   }
   else if (Type == _loginSuccessMsg) {

      if (InData) {

         String username;
         InData->lookup_string (_usernameHandle, 0, username);

         _log.warn << "LOGGED IN NOW AS: " << username << endl;
      }

      _loggedIn = True;
   }
   else if (Type == _loginFailedMsg) {

      if (_loginDialog) { _loginDialog->open (); }

      _loggedIn = False;
      _log.error << "FAILED TO LOGIN" << endl;
   }
}


void
dmz::QtPluginLoginDialog::_slot_dialog_accepted () {

   if (_loginDialog) {

      Data data;
_log.warn << "_setting username and password" << endl;
      String username = qPrintable (_ui.usernameLineEdit->text ());
      String password = qPrintable (_ui.passwordLineEdit->text ());

//      data.store_string (_usernameHandle, 0, "bordergame");
//      data.store_string (_passwordHandle, 0, "couch4me");

      data.store_string (_usernameHandle, 0, username);
      data.store_string (_passwordHandle, 0, password);

      _loginMsg.send (_targetHandle, &data);
   }
}


void
dmz::QtPluginLoginDialog::_slot_dialog_rejected () {

   _log.error << "dialog rejected" << endl;
}


// QtPluginLoginDialog Interface
void
dmz::QtPluginLoginDialog::_init (Config &local) {

   RuntimeContext *context (get_plugin_runtime_context ());

   _mainWindowModuleName = config_to_string ("module.main-window.name", local);

   _usernameHandle = config_to_named_handle (
      "username.name", local, "UsernameName", context);

   _passwordHandle = config_to_named_handle (
      "password.name", local, "PasswordName", context);

   _loginRequiredMsg = config_create_message (
      "message.login-required",
      local,
      "LoginRequiredMessage",
       context);

   _loginMsg = config_create_message (
      "message.login",
      local,
      "LoginMessage",
       context);

   _loginSuccessMsg = config_create_message (
      "message.login-success",
      local,
      "LoginSuccessMessage",
       context);

   _loginFailedMsg = config_create_message (
      "message.login-failed",
      local,
      "LoginFailedMessage",
       context);

   _logoutMsg = config_create_message (
      "message.logout",
      local,
      "LogoutMessage",
       context);

   subscribe_to_message (_loginRequiredMsg);
   subscribe_to_message (_loginSuccessMsg);
   subscribe_to_message (_loginFailedMsg);

   _targetHandle = config_to_named_handle ("target.name", local, context);
}


extern "C" {

DMZ_PLUGIN_FACTORY_LINK_SYMBOL dmz::Plugin *
create_dmzQtPluginLoginDialog (
      const dmz::PluginInfo &Info,
      dmz::Config &local,
      dmz::Config &global) {

   return new dmz::QtPluginLoginDialog (Info, local);
}

};
