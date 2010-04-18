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

   const Int32 ForgeTypeSearch             = 100;
   const Int32 ForgeTypeGetAsset           = 101;
   const Int32 ForgeTypePutAsset           = 102;
   const Int32 ForgeTypeDeleteAsset        = 103;
   const Int32 ForgeTypeGetAssetMedia      = 104;
   const Int32 ForgeTypePutAssetMedia      = 105;
   const Int32 ForgeTypeGetAssetPreview    = 106;
   const Int32 ForgeTypeAddAssetPreview    = 107;
   const Int32 ForgeTypeRemoveAssetPreview = 108;
   const Int32 ForgeTypeUser               = 1000;
         
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
