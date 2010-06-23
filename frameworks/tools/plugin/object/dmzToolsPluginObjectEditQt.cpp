#include <dmzObjectAttributeMasks.h>
#include <dmzObjectConsts.h>
#include <dmzObjectModule.h>
#include <dmzObjectModuleSelect.h>
#include <dmzRenderModuleIsect.h>
#include <dmzRuntimePluginFactoryLinkSymbol.h>
#include <dmzRuntimePluginInfo.h>
#include "dmzToolsPluginObjectEditQt.h"
#include <dmzTypesMath.h>
#include <dmzTypesMatrix.h>
#include <dmzTypesUUID.h>
#include <dmzTypesVector.h>

dmz::ToolsPluginObjectEditQt::ToolsPluginObjectEditQt (
      const PluginInfo &Info,
      Config &local) :
      Plugin (Info),
      MessageObserver (Info),
      ObjectObserverUtil (Info, local),
      _log (Info),
      _undo (Info),
      _inUpdate (False),
      _currentObject (0),
      _defaultAttrHandle (0),
      _selectAttrHandle (0),
      _select (0),
      _isect (0) {

   _ui.setupUi (this);

   setEnabled (false);

   show ();

   _init (local);
}


dmz::ToolsPluginObjectEditQt::~ToolsPluginObjectEditQt () {

}


// Plugin Interface
void
dmz::ToolsPluginObjectEditQt::update_plugin_state (
      const PluginStateEnum State,
      const UInt32 Level) {

   if (State == PluginStateInit) {

   }
   else if (State == PluginStateStart) {

   }
   else if (State == PluginStateStop) {

   }
   else if (State == PluginStateShutdown) {

   }
}


void
dmz::ToolsPluginObjectEditQt::discover_plugin (
      const PluginDiscoverEnum Mode,
      const Plugin *PluginPtr) {

   if (Mode == PluginDiscoverAdd) {

      if (!_select) { _select = ObjectModuleSelect::cast (PluginPtr); }
      if (!_isect) { _isect = RenderModuleIsect::cast (PluginPtr); }
   }
   else if (Mode == PluginDiscoverRemove) {

      if (_select && (_select == ObjectModuleSelect::cast (PluginPtr))) { _select = 0; }
      if (_isect && (_isect == RenderModuleIsect::cast (PluginPtr))) { _isect = 0; }
   }
}


// Message Observer Interface
void
dmz::ToolsPluginObjectEditQt::receive_message (
      const Message &Type,
      const UInt32 MessageSendHandle,
      const Handle TargetObserverHandle,
      const Data *InData,
      Data *outData) {

}


// Object Observer Interface
void
dmz::ToolsPluginObjectEditQt::destroy_object (
      const UUID &Identity,
      const Handle ObjectHandle) {

   if (ObjectHandle == _currentObject) {

      update_object_flag (Identity, ObjectHandle, _selectAttrHandle, False, 0);
   }
}

void
dmz::ToolsPluginObjectEditQt::update_object_flag (
      const UUID &Identity,
      const Handle ObjectHandle,
      const Handle AttributeHandle,
      const Boolean Value,
      const Boolean *PreviousValue) {

   Handle newObject (0);

   if (!_currentObject) {

      newObject = ObjectHandle;

   }
   else if (ObjectHandle == _currentObject) {

      if (!Value) {

         if (_select) {

            HandleContainer list;

            _select->get_selected_objects (list);

            HandleContainerIterator it;
            Handle next;

            while (!newObject && list.get_next (it, next)) {

               if (next != _currentObject) { newObject = next; }
            }
         }

         _currentObject = 0;
      }
      else {

         _log.error << "Selecting an object already selected." << endl;
      }
   }

   if (newObject) {

      _currentObject = newObject;

      setEnabled (true);

      ObjectModule *module (get_object_module ());

      if (module) {

         Vector pos;
         Matrix ori;

         UUID id;
         module->lookup_uuid (_currentObject, id),
         module->lookup_position (_currentObject, _defaultAttrHandle, pos);
         module->lookup_orientation (_currentObject, _defaultAttrHandle, ori);

         update_object_position (id, _currentObject, _defaultAttrHandle, pos, 0);
         update_object_orientation (id, _currentObject, _defaultAttrHandle, ori, 0);
      }
   }
   else if (!_currentObject) {

      setEnabled (false);
   }
}


void
dmz::ToolsPluginObjectEditQt::update_object_position (
      const UUID &Identity,
      const Handle ObjectHandle,
      const Handle AttributeHandle,
      const Vector &Value,
      const Vector *PreviousValue) {

   if (ObjectHandle == _currentObject) {

      _inUpdate = True;
      _ui.xedit->setValue (Value.get_x ());
      _ui.yedit->setValue (Value.get_y ());
      _ui.zedit->setValue (Value.get_z ());
      _inUpdate = False;
   }
}


void
dmz::ToolsPluginObjectEditQt::update_object_orientation (
      const UUID &Identity,
      const Handle ObjectHandle,
      const Handle AttributeHandle,
      const Matrix &Value,
      const Matrix *PreviousValue) {

   if (ObjectHandle == _currentObject) {

      _inUpdate = True;
      Float64 hy (0.0), px (0.0), rz (0.0);
      Value.to_euler_angles (hy, px, rz);
      hy = to_degrees (normalize_angle (hy, 0.0));
      px = to_degrees (normalize_angle (px, 0.0));
      rz = to_degrees (normalize_angle (rz, 0.0));
      _ui.hedit->setValue (hy);
      _ui.pedit->setValue (px);
      _ui.redit->setValue (rz);
      _ui.hdial->setValue ((int)hy * 2.0);
      _ui.pdial->setValue ((int)px * 2.0);
      _ui.rdial->setValue ((int)rz * 2.0);
      _inUpdate = False;
   }
}


// ToolsPluginObjectEditQt Interface
void
dmz::ToolsPluginObjectEditQt::on_xedit_valueChanged (double value) {

   if (!_inUpdate) {

      _inUpdate = True;
      _update_position ();
      _inUpdate = False;
   }
}


void
dmz::ToolsPluginObjectEditQt::on_yedit_valueChanged (double value) {

   if (!_inUpdate) {

      _inUpdate = True;
      _update_position ();
      _inUpdate = False;
   }
}


void
dmz::ToolsPluginObjectEditQt::on_zedit_valueChanged (double value) {

   if (!_inUpdate) {

      _inUpdate = True;
      _update_position ();
      _inUpdate = False;
   }
}


void
dmz::ToolsPluginObjectEditQt::on_hedit_valueChanged (double value) {

   if (!_inUpdate) {

      _inUpdate = True;

      _inUpdate = False;
   }
}


void
dmz::ToolsPluginObjectEditQt::on_pedit_valueChanged (double value) {

   if (!_inUpdate) {

      _inUpdate = True;

      _inUpdate = False;
   }
}


void
dmz::ToolsPluginObjectEditQt::on_redit_valueChanged (double value) {

   if (!_inUpdate) {

      _inUpdate = True;

      _inUpdate = False;
   }
}


void
dmz::ToolsPluginObjectEditQt::on_hdial_valueChanged (int value) {

   if (!_inUpdate) {

      _inUpdate = True;

      _inUpdate = False;
   }
}


void
dmz::ToolsPluginObjectEditQt::on_pdial_valueChanged (int value) {

   if (!_inUpdate) {

      _inUpdate = True;

      _inUpdate = False;
   }
}


void
dmz::ToolsPluginObjectEditQt::on_rdial_valueChanged (int value) {

   if (!_inUpdate) {

      _inUpdate = True;

      _inUpdate = False;
   }
}


void
dmz::ToolsPluginObjectEditQt::on_clampButton_pressed () {

}


void
dmz::ToolsPluginObjectEditQt::on_inPlaceCheckBox_stateChanged (int state) {

}


void
dmz::ToolsPluginObjectEditQt::on_autoClampCheckBox_stateChanged (int state) {

}


void
dmz::ToolsPluginObjectEditQt::_update_position () {


   ObjectModule *module (get_object_module ());

   if (_currentObject && module) {

      const Vector NewPos (
         _ui.xedit->value (),
         _ui.yedit->value (),
         _ui.zedit->value ());
_log.error << "Update position" << NewPos << endl;

      Vector pos;
      module->lookup_position (_currentObject, _defaultAttrHandle, pos);

      const Vector Offset = NewPos - pos;

      if (_select) {

         HandleContainer list;

         _select->get_selected_objects (list);

         HandleContainerIterator it;
         Handle next (0);

         while (list.get_next (it, next)) {

            Vector cpos;
            module->lookup_position (next, _defaultAttrHandle, cpos);
            module->store_position (next, _defaultAttrHandle, cpos + Offset);
         }
      }
   }
}


void
dmz::ToolsPluginObjectEditQt::_init (Config &local) {

   _defaultAttrHandle = activate_default_object_attribute (
      ObjectDestroyMask | ObjectPositionMask | ObjectOrientationMask);

   _selectAttrHandle = activate_object_attribute (
      ObjectAttributeSelectName,
      ObjectFlagMask);
}


extern "C" {

DMZ_PLUGIN_FACTORY_LINK_SYMBOL dmz::Plugin *
create_dmzToolsPluginObjectEditQt (
      const dmz::PluginInfo &Info,
      dmz::Config &local,
      dmz::Config &global) {

   return new dmz::ToolsPluginObjectEditQt (Info, local);
}

};
