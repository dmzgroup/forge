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

   const char ForgeSearchName[] = "Forge_Search";
   const char ForgeGetAssetName[] = "Forge_Get_Asset";
   const char ForgePutAssetName[] = "Forge_Put_Asset";
   const char ForgeGetAssetMediaName[] = "Forge_Get_Asset_Media";
   const char ForgeGetAssetPreviewName[] = "Forge_Get_Asset_Preview";
   
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
