#include "dmzQtExtAppShellResources.h"
#include <dmzApplication.h>
#include <dmzAppShellExt.h>
#include <dmzFoundationSHA.h>
#include <dmzRuntimeConfig.h>
#include <dmzRuntimeConfigToTypesBase.h>
#include <dmzRuntimeResourcesUpdate.h>
#include <dmzRuntimePluginFactoryLinkSymbol.h>
#include <dmzRuntimeResources.h>
#include <dmzTypesHashTableStringTemplate.h>
#include "ui_ProgressDialog.h"
#include <QtCore/QtCore>
#include <QtGui/QtGui>
#include <QtNetwork/QtNetwork>


using namespace dmz;

namespace {

static String
local_get_hash (const String &Version) {

   String hash;

   const Int32 Length = Version.get_length ();
   
   Int32 index (0);
   Int32 dashFound = -1;
   Boolean done (False);
   
   while (!done) {
   
      if (Version.get_char (index) == '-') { dashFound = index; done = True; }
      else { index++; if (index >= Length) { done = True; } }
   }
   
   Int32 dotFound = -1;
   index = Length - 1;
   done = False;
   
   while (!done) {
      
      if (Version.get_char (index) == '.') { dotFound = index; done = True; }
      else { index--; if (index < 0) { done = True; } }
   }
   
   if ((dashFound > 0) && (dotFound > 0)) {
      
      hash = Version.get_sub (dashFound + 1, dotFound -1);
   }
   
   return hash;
}


struct ResourceStruct {
   const String Name;
   const String Id;
   const String Version;
   String hash;
   String file;
   
   ResourceStruct (
         const String &TheName,
         const String &TheId,
         const String &TheVersion) :
         Name (TheName),
         Id (TheId),
         Version (TheVersion),
         hash (local_get_hash (TheVersion)),
         file (TheId + "-" + TheVersion) {;}
};

};


struct QtExtAppShellResources::State {
   
   AppShellResourcesStruct &resources;
   Application &app;
   Log log;
   String cacheDir;
   String forge;
   HashTableStringTemplate<ResourceStruct> resourceTable;
   QQueue<ResourceStruct *> downloadQueue;
   ResourceStruct *current;
   QNetworkAccessManager *networkAccessManager;
   QNetworkReply *reply;
   QTemporaryFile *downloadFile;
   QTime downloadTime;
   Int32 downloadCount;
   Int32 totalCount;
   Ui::ProgressDialog *ui;
   
   State (AppShellResourcesStruct &res);
   ~State ();
   
   Boolean init_cache_dir ();
   void build_resources_list (const Config &Resources);
   void update_resources ();
};


QtExtAppShellResources::State::State (AppShellResourcesStruct &res) :
      resources (res),
      app (res.app),
      log ("dmzQtExtAppShellResources", res.app.get_context ()),
      cacheDir (),
      forge ("http://api.dmzforge.org"),
      resourceTable (),
      downloadQueue (),
      current (0),
      networkAccessManager (0),
      reply (0),
      downloadFile (0),
      downloadTime (),
      downloadCount (0),
      totalCount (0),
      ui (0) {

}


QtExtAppShellResources::State::~State () {

   if (downloadFile) { delete downloadFile; downloadFile = 0;}

   resourceTable.empty ();
   downloadQueue.clear ();
}


Boolean
QtExtAppShellResources::State::init_cache_dir () {

   Boolean retVal (False);
   cacheDir = get_home_directory ();
   
   if (is_valid_path (cacheDir)) {

#if defined (_WIN32)
      cacheDir << "/Local Settings/Application Data/";
#elif defined (__APPLE__) || defined (MACOSX)
      cacheDir << "/Library/Caches/";
#else
      cacheDir << "/.";
#endif

      cacheDir = format_path (cacheDir + "dmz/" + app.get_name () + "/forge/");
      
      // create cache directory if it doesn't exists already
      if (!is_valid_path (cacheDir)) {
         
         create_directory (cacheDir);
         log.info << "Created applicatoin cache dir: " << cacheDir << endl;
      }
      
      retVal = True;
   }
   
   return retVal;
}


void
QtExtAppShellResources::State::build_resources_list (const Config &Resources) {
   
   Config list;
   if (Resources.lookup_all_config ("resource", list)) {
      
      ConfigIterator it;
      Config resource;
      
      while (list.get_next_config (it, resource)) {
         
         const String Name (config_to_string ("name", resource));
         const String Id (config_to_string ("id", resource));
         const String Version (config_to_string ("version", resource));
         
         if (Name && Id && Version) {
            
            ResourceStruct *rs = new ResourceStruct (Name, Id, Version);
            rs->file = format_path (cacheDir + rs->file);

            if (resourceTable.store (Name, rs)) {

               // if file not found on disk add it to download list
               if (!is_valid_path (rs->file)) {

                  downloadQueue.enqueue (rs);
                  totalCount++;
               }
            }
            else {
               
               delete rs; rs = 0;
               
               String msg ("Failed to store resource in table: ");
               msg << Name;
               app.set_error (msg);
            }
         }
         else {
            
            log.debug << "Skipping resource: " << Name << endl;
         }
      }
   }
}


void
QtExtAppShellResources::State::update_resources () {

   ResourcesUpdate updater (app.get_context ());

   HashTableStringIterator it;
   ResourceStruct *rs (resourceTable.get_first (it));

   while (rs) {
      
      log.debug << "Updating resource: " << rs->Name << endl;
      updater.update_resource_config (rs->Name, rs->file);
      rs = resourceTable.get_next (it);
   }
}


QtExtAppShellResources::QtExtAppShellResources (
      AppShellResourcesStruct &resources,
      QObject *parent) :
      QObject (parent),
      _state (*(new State (resources))) {

   Config global;
   _state.app.get_global_config (global);

   Config local;
   const String Prefix ("dmz.");
   global.lookup_all_config_merged (Prefix + resources.ExtName, local);
   _init (local);
   
   if (_state.init_cache_dir ()) {

      Config list;
      global.lookup_all_config_merged (Prefix + "runtime.resource-map", list);

      if (!list) {

         _state.app.log.info << "Runtime resource data not found" << endl;
      }
      else {

         _state.build_resources_list (list);
      }
   }
   else {
      
      _set_error ("Unable to init cache directory.");
   }
}


QtExtAppShellResources::~QtExtAppShellResources () {

   delete &_state;
}


void
QtExtAppShellResources::exec () {

   QDialog *dialog = new QDialog (0);
   
   _state.ui = new Ui::ProgressDialog;
   _state.ui->setupUi (dialog);
   
   connect (this, SIGNAL (error ()), dialog, SLOT (reject ()));
   connect (this, SIGNAL (finished ()), dialog, SLOT (accept ()));
   
   _state.ui->nameLabel->setText ("");
   _state.ui->infoLabel->setText ("");
   _state.ui->totalProgressBar->setMaximum (_state.totalCount);
   
   QTimer::singleShot (0, this, SLOT (_start_next_download ()));
   
   if (dialog->exec () == QDialog::Rejected) {

      _state.log.warn << "Updating resources aborted." << endl;
      _state.app.quit ("Updating resources aborted.");
   }
   
   if (_state.reply) {

      _state.reply->deleteLater ();
      _state.reply = 0;
   }

   if (_state.networkAccessManager) {
      
      _state.networkAccessManager->deleteLater ();
      _state.networkAccessManager = 0;
   }
   
   delete _state.ui; _state.ui = 0;
   delete dialog; dialog = 0;

   if (_state.app.is_error ()) {
   
      _state.app.quit ("Unable to update resources.");
   }
   else {
      
      _state.update_resources ();
   }
}


void
QtExtAppShellResources::_start_next_download () {

   if (_state.downloadQueue.isEmpty ()) {
      
      _state.ui->totalProgressBar->setValue (_state.totalCount);
      emit finished ();
   }
   else if (!_state.app.is_error ()){
      
      _state.current = _state.downloadQueue.dequeue ();

      if (_state.current) {
         
         if (_state.downloadFile) { delete _state.downloadFile; _state.downloadFile = 0; }
         
         _state.downloadFile = new QTemporaryFile (QDir::tempPath () + "/dmz_forge_download.XXXXXX");
         
         if (_state.downloadFile->open ()) {

            String forge (_state.forge);
            forge << "/assets/" << _state.current->Id << "/" << _state.current->Version;
            
            QNetworkRequest request;
            request.setUrl (QUrl (forge.get_buffer ()));
            request.setRawHeader("User-Agent", _state.app.get_name ().get_buffer ());
            
            // request.setRawHeader ("Accept", "application/json");
            // request.setHeader (QNetworkRequest::ContentTypeHeader, "application/json");
            
            // request.setRawHeader(
            //    "Authorization", "Basic " +
            //    QByteArray(QString("%1:%2").arg("user").arg("asas").toAscii()).toBase64());

            if (!_state.networkAccessManager) {
               
               _state.networkAccessManager = new QNetworkAccessManager (this);
            }
            
            _state.reply = _state.networkAccessManager->get (request);

            connect (
               _state.reply, SIGNAL (downloadProgress (qint64, qint64)),
               this, SLOT(_download_progress (qint64, qint64)));

            connect (
               _state.reply, SIGNAL (finished ()),
               this, SLOT(_download_finished ()));

            connect(
               _state.reply, SIGNAL (readyRead ()),
               this, SLOT (_download_ready_read ()));
            
            if (_state.ui) {

               _state.ui->totalProgressBar->setMaximum (_state.totalCount);
               _state.ui->totalProgressBar->setValue (_state.downloadCount);
               _state.ui->nameLabel->setText (_state.current->Version.get_buffer ());
               
               String text;
               text << "(" << _state.downloadCount + 1 << " of " << _state.totalCount << ")";
               // text << _state.totalCount - _state.downloadCount;
               _state.ui->countLabel->setText (text.get_buffer ());
            }
            
            _state.downloadTime.start ();
            _state.log.debug << "Downloading resource: " << forge << endl;
         }
         else if (_state.downloadFile) {

            _set_error (qPrintable (_state.downloadFile->errorString ()));
         }
      }
      else {
         
         _set_error ("Unable to dequeue next download.");
      }
   }
}


void
QtExtAppShellResources::_download_progress (qint64 bytesReceived, qint64 bytesTotal) {

   static const Float64 ToMB (1024.0*1024.0);
   Float64 received = bytesReceived / ToMB;
   Float64 total = bytesTotal / ToMB;

   // calculate the download speed
   Float64 speed = received * 1000.0 / _state.downloadTime.elapsed ();

   if (_state.ui) {

      _state.ui->currentProgressBar->setMaximum (bytesTotal);
      _state.ui->currentProgressBar->setValue (bytesReceived);
      
      QString text = QString("%1/%2 MB at %3 MB/s")
         .arg (received, 3, 'f', 2)
         .arg (total, 3, 'f', 2)
         .arg (speed, 3, 'f', 2);
      
      _state.ui->infoLabel->setText (text);
   }
}


void
QtExtAppShellResources::_download_finished () {
   
   if (_state.ui && _state.reply && _state.downloadFile && _state.current) {

      _state.ui->currentProgressBar->reset ();
      _state.ui->infoLabel->setText ("");
      _state.ui->nameLabel->setText ("");
   
      Boolean validDownload (False);
      
      if (_state.reply->error () != QNetworkReply::NoError) {
   
         _set_error (qPrintable (_state.reply->errorString ()));
      }
      else {

         validDownload = True;
         _state.downloadFile->close ();
         
         if (_state.current->hash) {
         
            const QString FileName (_state.downloadFile->fileName ());
            const String Hash (sha_from_file (qPrintable (FileName)));
         
            if (Hash == _state.current->hash) {

               _state.log.info << "Resource verified: " << _state.current->Name << endl;
            }
            else {

               validDownload = False;
               
               String msg ("Invalid hash for resource: ");
               msg << _state.current->Name;
               _set_error (msg);
            }
         }
         
         _state.downloadCount++;
      }
      
      if (validDownload) {
         
         _state.downloadFile->setAutoRemove (False);
         
         if (!_state.downloadFile->rename (_state.current->file.get_buffer ())) {
         
            _state.downloadFile->setAutoRemove (True);
            
            String msg ("Failed to rename downloaded file for resource: ");
            msg << _state.current->Name;
            _set_error (msg);
         }
      }
      
      delete _state.downloadFile;
      _state.downloadFile = 0;
      
      _state.reply->deleteLater ();
      _state.reply = 0;
      _state.current = 0;
      
      _start_next_download ();
   }
}


void
QtExtAppShellResources::_download_ready_read () {
   
   if (_state.reply && _state.downloadFile) {
      
      _state.downloadFile->write (_state.reply->readAll ());
   }
}


void
QtExtAppShellResources::_set_error (const String &Message) {
   
   _state.app.set_error (Message);
   emit error ();
}


void
QtExtAppShellResources::_init (Config &local) {

   _state.forge = config_to_string ("forge.url", local, _state.forge);
}


extern "C" {

DMZ_PLUGIN_FACTORY_LINK_SYMBOL void
dmz_resources_validate (AppShellResourcesStruct &resources) {

   QtExtAppShellResources validate (resources);
   validate.exec ();
}

};
