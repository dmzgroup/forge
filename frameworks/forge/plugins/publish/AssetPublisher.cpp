#include "AssetPublisher.h"
#include <dmzForgeConsts.h>
#include <dmzForgeModule.h>
#include <dmzQtUtil.h>
#include <QtGui/QtGui>


dmz::AssetPublisher::AssetPublisher (const PluginInfo &Info) :
      ForgeObserver (Info),
      _log (Info),
      _forge (0),
      _assetId (),
      _requestId (0),
      _media (),
      _previews (),
      _deleteAsset (False) {

}


dmz::AssetPublisher::~AssetPublisher () {

   _media.clear ();
   _previews.clear ();
}


void
dmz::AssetPublisher::setupUi (QWidget *parent) {

   Ui::PublishForm::setupUi (parent);

   QMovie *movie = loadingLabel->movie ();
   if (!movie) {

      movie = new QMovie (":/publish/loading.gif", QByteArray (), loadingLabel);
      loadingLabel->setMovie (movie);
   }
}


void
dmz::AssetPublisher::set_forge_module (ForgeModule *forge) {

   _forge = forge;
}


void
dmz::AssetPublisher::add_media (const String &File) {

   _media.append (File);
}


void
dmz::AssetPublisher::add_previews (const StringContainer &Container) {

   _previews = Container;
}


void
dmz::AssetPublisher::publish () {

   if (!_requestId && _forge) {

      String name (qPrintable (nameLineEdit->text ()));
      String brief (qPrintable (briefLineEdit->text ()));
      String details (qPrintable (detailsTextEdit->toPlainText ()));

      StringContainer keywords;
      to_dmz_string_container (keywordLineEdit->text ().split (" "), keywords);

      if (!_assetId) { _assetId = _forge->create_asset (); }

      _forge->store_name (_assetId, name);
      _forge->store_brief (_assetId, brief);
      _forge->store_details (_assetId, details);
      _forge->store_keywords (_assetId, keywords);

      _requestId = _forge->put_asset (_assetId, this);
_log.warn << "AssetID: " << _assetId << " : " << _requestId << endl;

      _begin ();
   }
}


void
dmz::AssetPublisher::cancel () {

   _log.warn << "Cancel upload" << endl;
   _deleteAsset = True;
   _end ();
}


void
dmz::AssetPublisher::reset () {

   _end ();

   _assetId.flush ();

   nameLineEdit->clear ();
   briefLineEdit->clear ();
   detailsTextEdit->clear ();
}


// ForgeObserver Interface
void
dmz::AssetPublisher::handle_reply (
      const UInt64 RequestId,
      const Int32 ReqeustType,
      const Boolean Error,
      const StringContainer &Results) {

   switch (ReqeustType) {

      case ForgeTypeSearch:
         break;

      case ForgeTypeGetAsset:
         break;

      case ForgeTypePutAsset:
         if (RequestId == _requestId) { _publish_media (); }
         break;

      case ForgeTypeDeleteAsset:
         break;

      case ForgeTypeGetAssetMedia:
         break;

      case ForgeTypePutAssetMedia:
         if (RequestId == _requestId) { _publish_media (); }
         break;

      case ForgeTypeAddAssetPreview:
         if (RequestId == _requestId) { _publish_finished (); }
         break;

      default:
         break;
   }

   if (Error) {

_log.error << "Results: " << Results << endl;
      String error;
      Results.get_first (error);
      _publish_error (error);
   }
}


void
dmz::AssetPublisher::_publish_media () {

   if (_media.get_count ()) {

      if (_assetId && _forge) {

         String file;
         _media.get_first (file);
         _media.remove (file);

_log.warn << "_publish_media: " << file << endl;

         _requestId = _forge->put_asset_media (_assetId, file, this);
      }
   }
   else { _publish_previews (); }
}


void
dmz::AssetPublisher::_publish_previews () {

   if (_previews.get_count ()) {

      if (_assetId && _forge) {

_log.warn << "_publish_previews: " << _previews.get_count () << endl;

         _requestId = _forge->add_asset_preview (_assetId, _previews, this);
      }
   }
   else { _publish_finished (); }
}


void
dmz::AssetPublisher::_publish_finished () {

_log.error << "Publish Finished: " << _requestId << endl;
   _end ();
}


void
dmz::AssetPublisher::_publish_error (const String &Error) {

_log.error << "Publish ERROR: " << _requestId  << " : " << Error << endl;

   _deleteAsset = True;
   _end ();
}


void
dmz::AssetPublisher::_begin () {

   buttonBox->setEnabled (False);
   statusFrame->show ();
   loadingLabel->movie ()->start ();
}


void
dmz::AssetPublisher::_end () {

   _requestId = 0;

   _media.clear ();
   _previews.clear ();

   buttonBox->setEnabled (True);
   statusFrame->hide ();

   loadingLabel->movie ()->stop ();
}
