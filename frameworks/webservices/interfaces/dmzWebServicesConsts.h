#ifndef DMZ_WEBSERVICES_CONSTS_DOT_H
#define DMZ_WEBSERVICES_CONSTS_DOT_H

#include <dmzTypesBase.h>
// #include <dmzTypesString.h>

/*!

\file
\ingroup WebServices
\brief Defines constants.

*/

namespace dmz {

//! \addtogroup WebServices
//! @{

   const char WebServicesArchiveName[] = "WebServices_Archive";
   const char WebServicesPublishAttributeName[] = "WebServices_Publish";
   const char WebServicesRevisionAttributeName[] = "_rev";
   
   const Int32 WebServiceTypePublishObject   = 100;
   const Int32 WebServiceTypeUser            = 1000;
};


#endif // DMZ_WEBSERVICES_CONSTS_DOT_H
