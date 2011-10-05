#ifndef DMZ_QT_PLUGIN_LOGIN_CHDS_DOT_H
#define DMZ_QT_PLUGIN_LOGIN_CHDS_DOT_H

#include <dmzRuntimeConfig.h>
#include <dmzRuntimeLog.h>
#include <dmzRuntimeMessaging.h>
#include <dmzRuntimePlugin.h>
#include <QtCore/QObject>
#include "ui_dmzQtPluginLoginChds.h"

class QDialog;
class QButtonGroup;


namespace dmz {

   class QtModuleMainWindow;

   class QtPluginLoginChds :
         public QObject,
         public Plugin,
         public MessageObserver {

      Q_OBJECT

      public:
         QtPluginLoginChds (const PluginInfo &Info, Config &local);
         ~QtPluginLoginChds ();

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
         void _slot_update_dialog ();

      protected:
         // QtPluginLoginChds Interface
         void _create_dialog (QWidget *parent);
         void _init_pictures ();
         void _init_colors ();
         void _init (Config &local);

         Log _log;
         QtModuleMainWindow *_mainWindowModule;
         String _mainWindowModuleName;
         Handle _nameHandle;
         Handle _passwordHandle;
         Handle _targetHandle;
         Handle _databaseHandle;
         Message _loginRequiredMsg;
         Message _loginMsg;
         Message _loginSuccessMsg;
         Message _loginFailedMsg;
         Message _loginSkippedMsg;
         Message _waitToOpenMsg;
         Message _allowOpenMsg;
         Boolean _waitToOpen;
         Boolean _triedToOpen;
         Boolean _loggedIn;
         QDialog *_loginDialog;
         Ui::LoginDialog _ui;
         Config _pictureConfig;
         QButtonGroup *_pictureGroup;
         Config _colorConfig;
         QButtonGroup *_colorGroup;
         String _user;
         String _database;
         String _defaultDatabase;
         String _lastDB;
         String _color;
         String _picture;

      private:
         QtPluginLoginChds ();
         QtPluginLoginChds (const QtPluginLoginChds &);
         QtPluginLoginChds &operator= (const QtPluginLoginChds &);

   };
};

#endif // DMZ_QT_PLUGIN_LOGIN_CHDS_DOT_H
