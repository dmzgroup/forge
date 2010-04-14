#ifndef DMZ_FORGE_CONSTS_DOT_H
#define DMZ_FORGE_CONSTS_DOT_H

#include <dmzTypesBase.h>
// #include <dmzTypesString.h>

/*!

\file
\ingroup Forge
\brief Defines constants.

*/

namespace dmz {

//! \addtogroup Forge
//! @{

   enum ForgeTypeEnum {
      ForgeSearch = 101,
      ForgeGetAsset,
      ForgePutAsset,
      ForgeDeleteAsset,
      ForgeGetAssetMedia,
      ForgePutAssetMedia,
      ForgeGetAssetPreview,
      ForgePutAssetPreview
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
