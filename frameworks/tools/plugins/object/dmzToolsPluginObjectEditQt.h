#ifndef DMZ_TOOLS_PLUGIN_OBJECT_EDIT_QT_DOT_H
#define DMZ_TOOLS_PLUGIN_OBJECT_EDIT_QT_DOT_H

#include <dmzObjectObserverUtil.h>
#include <dmzRuntimeLog.h>
#include <dmzRuntimeMessaging.h>
#include <dmzRuntimePlugin.h>
#include <dmzRuntimeUndo.h>

#include <QtGui/QFrame>
#include <ui_ObjectAttributeEditor.h>

namespace dmz {

   class ObjectModuleSelect;
   class RenderModuleIsect;

   class ToolsPluginObjectEditQt :
         public QFrame,
         public Plugin,
         public MessageObserver,
         public ObjectObserverUtil {

      Q_OBJECT

      public:
         ToolsPluginObjectEditQt (const PluginInfo &Info, Config &local);
         ~ToolsPluginObjectEditQt ();

         // Plugin Interface
         virtual void update_plugin_state (
            const PluginStateEnum State,
            const UInt32 Level);

         virtual void discover_plugin (
            const PluginDiscoverEnum Mode,
            const Plugin *PluginPtr);

         // Message Observer Interface
         virtual void receive_message (
            const Message &Type,
            const UInt32 MessageSendHandle,
            const Handle TargetObserverHandle,
            const Data *InData,
            Data *outData);

         // Object Observer Interface
         virtual void destroy_object (
            const UUID &Identity,
            const Handle ObjectHandle);

         virtual void update_object_flag (
            const UUID &Identity,
            const Handle ObjectHandle,
            const Handle AttributeHandle,
            const Boolean Value,
            const Boolean *PreviousValue);

         virtual void update_object_position (
            const UUID &Identity,
            const Handle ObjectHandle,
            const Handle AttributeHandle,
            const Vector &Value,
            const Vector *PreviousValue);

         virtual void update_object_orientation (
            const UUID &Identity,
            const Handle ObjectHandle,
            const Handle AttributeHandle,
            const Matrix &Value,
            const Matrix *PreviousValue);

      protected slots:
         // ToolsPluginObjectEditQt Interface
         void on_xedit_valueChanged (double value);
         void on_yedit_valueChanged (double value);
         void on_zedit_valueChanged (double value);

         void on_fbutton_pressed ();
         void on_rbutton_pressed ();
         void on_lbutton_pressed ();
         void on_bbutton_pressed ();

         void on_stepEdit_valueChanged (double value);

         void on_hedit_valueChanged (double value);
         void on_pedit_valueChanged (double value);
         void on_redit_valueChanged (double value);

         void on_hdial_valueChanged (int value);
         void on_pdial_valueChanged (int value);
         void on_rdial_valueChanged (int value);

         void on_clampButton_pressed ();

         void on_autoClampCheckBox_stateChanged (int state);

      protected:
         void _move (const Vector &Value);
         void _clamp (const Handle Object);
         void _update_position ();
         void _update_orientation ();
         void _init (Config &local);

         Log _log;
         Undo _undo;

         Float64 _stepValue;

         Boolean _inUpdate;

         Handle _defaultAttrHandle;
         Handle _selectAttrHandle;
         Handle _currentObject;

         ObjectModuleSelect *_select;
         RenderModuleIsect *_isect;

         Ui::ObjectAttributes _ui;

      private:
         ToolsPluginObjectEditQt ();
         ToolsPluginObjectEditQt (const ToolsPluginObjectEditQt &);
         ToolsPluginObjectEditQt &operator= (const ToolsPluginObjectEditQt &);

   };
};

#endif // DMZ_TOOLS_PLUGIN_OBJECT_EDIT_QT_DOT_H
