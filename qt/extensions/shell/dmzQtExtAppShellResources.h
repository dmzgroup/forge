#ifndef DMZ_QT_EXT_APP_SHELL_RESOURCES_DOT_H
#define DMZ_QT_EXT_APP_SHELL_RESOURCES_DOT_H

#include <dmzAppShellExt.h>
#include <QtCore/QObject>

namespace dmz {

class QtExtAppShellResources : public QObject {

   Q_OBJECT

   public:
      QtExtAppShellResources (AppShellResourcesStruct &resources, QObject *parent = 0);
      ~QtExtAppShellResources ();
      
      void exec ();

   signals:
      void finished ();
      void error ();

   protected slots:
      void _start_next_download ();
      void _download_progress (qint64 bytesReceived, qint64 bytesTotal);
      void _download_finished ();
      void _download_ready_read ();
         
   protected:
      void _set_error (const String &Message);
      void _init (Config &local);
      
      struct State;
      State &_state;
};

};

#endif // DMZ_QT_EXT_APP_SHELL_RESOURCES_DOT_H
