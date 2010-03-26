#ifndef DMZ_FORGE_CONSTS_DOT_H
#define DMZ_FORGE_CONSTS_DOT_H

#include <dmzTypesBase.h>
#include <dmzTypesString.h>

/*!

\file
\ingroup Forge
\brief Defines constants.

*/

namespace dmz {

//! \addtogroup Forge
//! @{

   //! Asset typeObject locality enumeration. Defined in dmzObjectConsts.h.
   enum AssetTypeEnum {
      ObjectLocalityUnknown, //!< Unknown Locality.
      ObjectLocal,           //!< Object is local.
      ObjectRemote,          //!< Object is remote.
   };

   //! Forge event enumeraion. Definded in dmzForgeConsts.h.
   enum ForgeEventEnum {
      ForgeEventSearch,        //!< search event.
      ForgeEventGetAsset,      //!< get_asset event.
      ForgeEventPutAsset,      //!< put_asset event.
      ForgeEventDeleteAsset,   //!< delete_asset event.
      ForgeEventGetAssetMedia, //!< get_asset_media event.
      ForgeEventPutAssetMedia, //!< put_asset_media event.
      ForgeEventGetPreview,    //!< get_preview event.
      ForgeEventPutPreview     //!< put_preview event.
   };
   
   //! Forge media asset type. Defined in dmzForgeConsts.h.
   enum ForgeAssetTypeEnum {
      ForgeAssetUnknown, //!< Unknown type.
      ForgeAsset3d,      //!< 3D.
      ForgeAssetAudio,   //!< Audio.
      ForgeAssetImage,   //!< Image.
      ForgeAssetConfig   //!< Config.
   };
};


#endif // DMZ_FORGE_CONSTS_DOT_H
