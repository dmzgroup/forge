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

   const char WebServicesLoginRequiredMessageName[] = "Login_Required_Message";
   const char WebServicesLoginMessageName[]         = "Login_Message";
   const char WebServicesLoginSuccessMessageName[]  = "Login_Success_Message";
   const char WebServicesLoginFailedMessageName[]   = "Login_Failed_Message";
   const char WebServicesLogoutMessageName[]        = "Logout_Message";

   const char WebServicesAttributeAdminName[]       = "admin";
   const char WebServicesAttributeUserName[]        = "name";
   const char WebServicesAttributePasswordName[]    = "password";
   const char WebServicesAttributeTimeStampName[]   = "time-stamp";
   const char WebServicesAttributeIdName[]          = "_id";
   const char WebServicesAttributeRevisionName[]    = "_rev";
   const char WebServicesAttributeTypeName[]        = "type";
};


#endif // DMZ_WEBSERVICES_CONSTS_DOT_H
