#include <dmzQtConfigRead.h>
#include <dmzQtConfigWrite.h>
#include <dmzQtModuleMainWindow.h>
#include "dmzQtPluginLoginDialog.h"
#include <dmzRuntimeConfigToNamedHandle.h>
#include <dmzRuntimeConfigToTypesBase.h>
#include <dmzRuntimeConfigWrite.h>
#include <dmzRuntimeData.h>
#include <dmzRuntimePluginFactoryLinkSymbol.h>
#include <dmzRuntimePluginInfo.h>
#include <dmzRuntimeSession.h>
#include <QtGui/QtGui>

namespace {

QByteArray
local_calc_xor (const QByteArray &Data, const QByteArray &Key) {

   QByteArray result;

   if (Key.isEmpty ()) { result = Data; }
   else {

      for (int ix = 0 , jx = 0; ix < Data.length (); ++ix , ++jx) {

         if (jx == Key.length ()) { jx = 0; }
         result.append (Data.at (ix) ^ Key.at (jx));
      }
   }

    return result;
}

};

dmz::QtPluginLoginDialog::QtPluginLoginDialog (const PluginInfo &Info, Config &local) :
      QObject (0),
      Plugin (Info),
      MessageObserver (Info),
      _log (Info),
      _mainWindowModule (0),
      _mainWindowModuleName (),
      _nameHandle (0),
      _passwordHandle (0),
      _targetHandle (0),
      _loggedIn (False),
      _autoLogin (False),
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

   const QByteArray Key (get_plugin_name ().get_buffer ());
   RuntimeContext *context (get_plugin_runtime_context ());

   if (State == PluginStateInit) {

      Config session (get_session_config (get_plugin_name (), context));

      if (_loginDialog) {

         if (config_to_boolean ("remember.me", session)) {

            _ui.rememberMe->setChecked (True);

            String username = config_to_string ("user.name", session);
            _ui.usernameLineEdit->setText (username.get_buffer ());

            QByteArray ba (config_to_qbytearray ("password", session));
            QByteArray password = local_calc_xor (ba, Key.toBase64 ());

            _ui.passwordLineEdit->setText (password);
         }
      }
   }
   else if (State == PluginStateStart) {

   }
   else if (State == PluginStateStop) {

   }
   else if (State == PluginStateShutdown) {

      if (_loginDialog) {

         Config session (get_plugin_name ());

         Boolean rememberMe (_ui.rememberMe->isChecked ());
         if (rememberMe) {

            String username = qPrintable (_ui.usernameLineEdit->text ());
            session.add_config (string_to_config ("user", "name", username));

            QByteArray password (qPrintable (_ui.passwordLineEdit->text ()));

            QByteArray ba = local_calc_xor (password, Key.toBase64 ());
            session.add_config (qbytearray_to_config ("password", ba));
         }

         session.add_config (boolean_to_config ("remember", "me", rememberMe));

         set_session_config (context, session);
      }
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

      if (_loginDialog) {

         if (_autoLogin) { _slot_dialog_accepted (); }
         else { _loginDialog->open (); }
      }
   }
   else if (Type == _loginSuccessMsg) {

      if (InData) {

         String username;
         InData->lookup_string (_nameHandle, 0, username);
      }

      _loggedIn = True;
   }
   else if (Type == _loginFailedMsg) {

      _loggedIn = False;

      if (_loginDialog) { _loginDialog->open (); }
   }
}


void
dmz::QtPluginLoginDialog::_slot_dialog_accepted () {

   if (_loginDialog) {

      Data data;

      String username = qPrintable (_ui.usernameLineEdit->text ());
      String password = qPrintable (_ui.passwordLineEdit->text ());

      data.store_string (_nameHandle, 0, username);
      data.store_string (_passwordHandle, 0, password);

      _loginMsg.send (_targetHandle, &data);
   }
}


void
dmz::QtPluginLoginDialog::_slot_dialog_rejected () {

   // _log.error << "dialog rejected" << endl;
}


// QtPluginLoginDialog Interface
void
dmz::QtPluginLoginDialog::_init (Config &local) {

   RuntimeContext *context (get_plugin_runtime_context ());

   _mainWindowModuleName = config_to_string ("module.main-window.name", local);

   _nameHandle = config_to_named_handle (
      "attribute.name", local, "name", context);

   _passwordHandle = config_to_named_handle (
      "attribute.password", local, "password", context);

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

   _targetHandle = config_to_named_handle ("login-target.name", local, context);

   _autoLogin = config_to_boolean ("auto-login.value", local, _autoLogin);
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
