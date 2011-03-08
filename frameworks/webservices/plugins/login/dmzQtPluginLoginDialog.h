#ifndef DMZ_QT_PLUGIN_LOGIN_DIALOG_DOT_H
#define DMZ_QT_PLUGIN_LOGIN_DIALOG_DOT_H

#include <dmzRuntimeLog.h>
#include <dmzRuntimeMessaging.h>
#include <dmzRuntimePlugin.h>
#include <QtCore/QObject>
#include "ui_dmzQtPluginLoginDialog.h"

class QDialog;


namespace dmz {

   class QtModuleMainWindow;

   class QtPluginLoginDialog :
         public QObject,
         public Plugin,
         public MessageObserver {

      Q_OBJECT

      public:
         QtPluginLoginDialog (const PluginInfo &Info, Config &local);
         ~QtPluginLoginDialog ();

         // Plugin Interface
         virtual void update_plugin_state (
            const PluginStateEnum State,
            const UInt32 Level);

         virtual void discover_plugin (
            const PluginDiscoverEnum Mode,
            const Plugin *PluginPtr);

         // Message Observer Interface
         virtual void receive_message (
            const Message &Type,
            const UInt32 MessageSendHandle,
            const Handle TargetObserverHandle,
            const Data *InData,
            Data *outData);

      protected Q_SLOTS:
         void _slot_dialog_accepted ();
         void _slot_dialog_rejected ();

      protected:
         // QtPluginLoginDialog Interface
         void _init (Config &local);

         Log _log;
         QtModuleMainWindow *_mainWindowModule;
         String _mainWindowModuleName;
         Handle _nameHandle;
         Handle _passwordHandle;
         Handle _targetHandle;
         Message _loginRequiredMsg;
         Message _loginMsg;
         Message _loginSuccessMsg;
         Message _loginFailedMsg;
         Message _logoutMsg;
         Boolean _loggedIn;
         Boolean _autoLogin;
         QDialog *_loginDialog;
         Ui::LoginDialog _ui;

      private:
         QtPluginLoginDialog ();
         QtPluginLoginDialog (const QtPluginLoginDialog &);
         QtPluginLoginDialog &operator= (const QtPluginLoginDialog &);

   };
};

#endif // DMZ_QT_PLUGIN_LOGIN_DIALOG_DOT_H
