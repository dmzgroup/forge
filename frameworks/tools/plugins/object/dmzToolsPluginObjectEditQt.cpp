#include <dmzObjectAttributeMasks.h>
#include <dmzObjectConsts.h>
#include <dmzObjectModule.h>
#include <dmzObjectModuleSelect.h>
#include <dmzQtConfigRead.h>
#include <dmzQtConfigWrite.h>
#include <dmzRenderIsectUtil.h>
#include <dmzRuntimeConfigWrite.h>
#include <dmzRenderModuleIsect.h>
#include <dmzRuntimePluginFactoryLinkSymbol.h>
#include <dmzRuntimePluginInfo.h>
#include <dmzRuntimeSession.h>
#include "dmzToolsPluginObjectEditQt.h"
#include <dmzTypesMath.h>
#include <dmzTypesMatrix.h>
#include <dmzTypesUUID.h>
#include <dmzTypesVector.h>

namespace {

static const dmz::Vector Up (0.0, 1.0, 0.0);

};

dmz::ToolsPluginObjectEditQt::ToolsPluginObjectEditQt (
      const PluginInfo &Info,
      Config &local) :
      Plugin (Info),
      MessageObserver (Info),
      ObjectObserverUtil (Info, local),
      _log (Info),
      _undo (Info),
      _stepValue (1.0),
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

   _stepValue = _ui.stepEdit->singleStep ();
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

      RuntimeContext *context (get_plugin_runtime_context ());

      if (context) {

         Config session (get_plugin_name ());

         session.add_config (qbytearray_to_config ("geometry", saveGeometry ()));

         if (isVisible ()) {

            session.add_config (boolean_to_config ("window", "visible", True));
         }

         set_session_config (context, session);
      }
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

   if (!_currentObject && Value) {

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

      _inUpdate = True;
      _ui.xedit->setValue (0.0);
      _ui.yedit->setValue (0.0);
      _ui.zedit->setValue (0.0);

      _ui.hedit->setValue (0.0);
      _ui.pedit->setValue (0.0);
      _ui.redit->setValue (0.0);

      _ui.hdial->setValue (0);
      _ui.pdial->setValue (0);
      _ui.rdial->setValue (0);
      _inUpdate = False;
   }
}


void
dmz::ToolsPluginObjectEditQt::update_object_position (
      const UUID &Identity,
      const Handle ObjectHandle,
      const Handle AttributeHandle,
      const Vector &Value,
      const Vector *PreviousValue) {

   if (!_inUpdate && (ObjectHandle == _currentObject)) {

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

   if (!_inUpdate && (ObjectHandle == _currentObject)) {

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
dmz::ToolsPluginObjectEditQt::on_fbutton_pressed () {

   const Vector Value (0.0, 0.0, -_stepValue);
   _move (Value);
}


void
dmz::ToolsPluginObjectEditQt::on_rbutton_pressed () {

   const Vector Value (_stepValue, 0.0, 0.0);
   _move (Value);
}


void
dmz::ToolsPluginObjectEditQt::on_lbutton_pressed () {

   const Vector Value (-_stepValue, 0.0, 0.0);
   _move (Value);
}


void
dmz::ToolsPluginObjectEditQt::on_bbutton_pressed () {

   const Vector Value (0.0, 0.0, _stepValue);
   _move (Value);
}


void
dmz::ToolsPluginObjectEditQt::on_stepEdit_valueChanged (double value) {

   _stepValue = value;

   _ui.xedit->setSingleStep (value);
   _ui.yedit->setSingleStep (value);
   _ui.zedit->setSingleStep (value);
}


void
dmz::ToolsPluginObjectEditQt::on_hedit_valueChanged (double value) {

   if (!_inUpdate) {

      _inUpdate = True;
      _update_orientation ();
      _inUpdate = False;
   }
}


void
dmz::ToolsPluginObjectEditQt::on_pedit_valueChanged (double value) {

   if (!_inUpdate) {

      _inUpdate = True;
      _update_orientation ();
      _inUpdate = False;
   }
}


void
dmz::ToolsPluginObjectEditQt::on_redit_valueChanged (double value) {

   if (!_inUpdate) {

      _inUpdate = True;
      _update_orientation ();
      _inUpdate = False;
   }
}


void
dmz::ToolsPluginObjectEditQt::on_hdial_valueChanged (int value) {

   if (!_inUpdate) { _ui.hedit->setValue ((double)(value) * 0.5); }
}


void
dmz::ToolsPluginObjectEditQt::on_pdial_valueChanged (int value) {

   if (!_inUpdate) { _ui.pedit->setValue ((double)(value) * 0.5); }
}


void
dmz::ToolsPluginObjectEditQt::on_rdial_valueChanged (int value) {

   if (!_inUpdate) { _ui.redit->setValue ((double)(value) * 0.5); }
}


void
dmz::ToolsPluginObjectEditQt::on_clampButton_pressed () {

   if (_select) {

      HandleContainer list;

      _select->get_selected_objects (list);

      HandleContainerIterator it;
      Handle next (0);

      while (list.get_next (it, next)) { _clamp (next); }
   }
}


void
dmz::ToolsPluginObjectEditQt::on_autoClampCheckBox_stateChanged (int state) {

   const bool Mode (state != Qt::Checked);

   _ui.pedit->setEnabled (Mode);
   _ui.pdial->setEnabled (Mode);

   _ui.redit->setEnabled (Mode);
   _ui.rdial->setEnabled (Mode);

   if (!Mode) { on_clampButton_pressed (); }
}


void
dmz::ToolsPluginObjectEditQt::_move (const Vector &Value) {

   ObjectModule *module (get_object_module ());

   if (module) {

      Boolean doClamp = False;

      if (_ui.autoClampCheckBox->checkState () == Qt::Checked) { doClamp = True; }

      if (_select) {

         HandleContainer list;

         _select->get_selected_objects (list);

         HandleContainerIterator it;
         Handle next (0);

         Handle undo (0);

         while (list.get_next (it, next)) {

            if (!undo) { undo = _undo.start_record ("Move Object(s)"); }

            Vector pos;
            Matrix ori;
            module->lookup_position (next, _defaultAttrHandle, pos);
            module->lookup_orientation (next, _defaultAttrHandle, ori);
            Vector offset (Value);
            ori.transform_vector (offset);
            module->store_position (next, _defaultAttrHandle, pos + offset);
            if (doClamp) { _clamp (next); }
         }

         if (undo) { _undo.stop_record (undo); }
      }
   }
}


void
dmz::ToolsPluginObjectEditQt::_clamp (const Handle Object) {

   ObjectModule *module (get_object_module ());

   if (module && _isect) {

      Vector pos, normal;
      Matrix ori;
      module->lookup_position (Object, _defaultAttrHandle, pos);
      module->lookup_orientation (Object, _defaultAttrHandle, ori);

      _isect->disable_isect (Object);
      isect_clamp_point (pos, *_isect, pos, normal);
      _isect->enable_isect (Object);

      if (!normal.is_zero ()) {

         Float64 hy (0.0), px (0.0), rz (0.0);
         ori.to_euler_angles (hy, px, rz);
         ori.from_axis_and_angle (Up, hy);
         Matrix tilt (Up, normal);

         ori = tilt * ori;
      }

      const Handle Undo = _undo.start_record ("Clamp Object");
      module->store_position (Object, _defaultAttrHandle, pos);
      module->store_orientation (Object, _defaultAttrHandle, ori);
      _undo.stop_record (Undo);
   }
}


void
dmz::ToolsPluginObjectEditQt::_update_position () {

   ObjectModule *module (get_object_module ());

   if (_currentObject && module) {

      Boolean doClamp = False;

      if (_ui.autoClampCheckBox->checkState () == Qt::Checked) { doClamp = True; }

      const Vector NewPos (
         _ui.xedit->value (),
         _ui.yedit->value (),
         _ui.zedit->value ());

      Vector pos;
      module->lookup_position (_currentObject, _defaultAttrHandle, pos);

      const Vector Offset = NewPos - pos;

      if (_select) {

         HandleContainer list;

         _select->get_selected_objects (list);

         HandleContainerIterator it;
         Handle next (0);

         Handle undo (0);

         while (list.get_next (it, next)) {

            if (!undo) { undo = _undo.start_record ("Move Object(s)"); }

            Vector cpos;
            module->lookup_position (next, _defaultAttrHandle, cpos);
            module->store_position (next, _defaultAttrHandle, cpos + Offset);
            if (doClamp) { _clamp (next); }
         }

         if (undo) { _undo.stop_record (undo); }
      }
   }
}


void
dmz::ToolsPluginObjectEditQt::_update_orientation () {

   ObjectModule *module (get_object_module ());

   if (_currentObject && module) {

      Boolean doClamp = False;

      if (_ui.autoClampCheckBox->checkState () == Qt::Checked) { doClamp = True; }

      const Float64 NewH (to_radians (_ui.hedit->value ())),
         NewP (to_radians (_ui.pedit->value ())),
         NewR (to_radians (_ui.redit->value ()));

      _ui.hdial->setValue ((int)(_ui.hedit->value () * 2.0));
      _ui.pdial->setValue ((int)(_ui.pedit->value () * 2.0));
      _ui.rdial->setValue ((int)(_ui.redit->value () * 2.0));

      Matrix ori;
      module->lookup_orientation (_currentObject, _defaultAttrHandle, ori);

      Float64 hy (0.0), px (0.0), rz (0.0);

      ori.to_euler_angles (hy, px, rz);

      hy = NewH - hy;
      px = NewP - px;
      rz = NewR - rz;

      if (_select) {

         HandleContainer list;

         _select->get_selected_objects (list);

         HandleContainerIterator it;
         Handle next (0);

         Handle undo (0);

         while (list.get_next (it, next)) {

            if (!undo) { undo = _undo.start_record ("Rotate Object(s)"); }

            module->lookup_orientation (next, _defaultAttrHandle, ori);

            Float64 chy (0.0), cpx (0.0), crz (0.0);
            ori.to_euler_angles (chy, cpx, crz);
            ori.from_euler_angles (chy + hy, cpx + px, crz + rz);
            module->store_orientation (next, _defaultAttrHandle, ori);
            if (doClamp) { _clamp (next); }
         }

         if (undo ) { _undo.stop_record (undo); }
      }
   }
}


void
dmz::ToolsPluginObjectEditQt::_init (Config &local) {

   RuntimeContext *context = get_plugin_runtime_context ();

   if (context) {

      Config session (get_session_config (get_plugin_name (), context));

      Config gdata;

      if (session.lookup_config ("geometry", gdata)) {

         restoreGeometry (config_to_qbytearray (gdata));
      }

      if (config_to_boolean ("window.visible", session, False)) { show (); }
   }

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
