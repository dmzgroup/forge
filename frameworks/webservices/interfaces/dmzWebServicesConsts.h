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

   const char WebServicesLoginRequiredMessageName[] = "LoginRequiredMessage";
   const char WebServicesLoginMessageName[]         = "LoginMessage";
   const char WebServicesLoginSuccessMessageName[]  = "LoginSuccessMessage";
   const char WebServicesLoginFailedMessageName[]   = "LoginFailedMessage";
   const char WebServicesLogoutMessageName[]        = "LogoutMessage";

   const char WebServicesAttributeUserName[]        = "name";
   const char WebServicesAttributePassword[]        = "password";
   const char WebServicesAttributeIdName[]          = "_id";
   const char WebServicesAttributeRevName[]         = "_rev";
   const char WebServicesAttributeTypeName[]        = "type";
};


#endif // DMZ_WEBSERVICES_CONSTS_DOT_H
