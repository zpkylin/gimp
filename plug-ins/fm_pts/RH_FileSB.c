//
// README: Portions of this file are merged at file generation
// time. Edits can be made *only* in between specified code blocks, look
// for keywords <Begin user code> and <End user code>.
//
/////////////////////////////////////////////////////////////
//
// Source file for RH_FileSB
//
//    This class is a ViewKit "component", as described
//    in the ViewKit Programmer's Guide.
//
// This file created with:
//
//    Builder Xcessory Version 5.0.5
//    Code Generator Xcessory 5.0.2 (03/15/99) Script Version 5.0.5
//
/////////////////////////////////////////////////////////////


// Begin user code block <file_comments>
// End user code block <file_comments>

#include <Vk/VkApp.h>
#include <X11/StringDefs.h>
#include <Vk/VkResource.h>
#include <Xm/Form.h>
#include <Xm/FileSB.h>
#include "RH_FileSB.h"

//
// Convenience functions from utilities file.
//
extern void RegisterBxConverters(XtAppContext);
extern XtPointer BX_CONVERT(Widget, char *, char *, int, Boolean *);
extern XtPointer BX_DOUBLE(double);
extern XtPointer BX_SINGLE(float);
extern Pixmap XPM_PIXMAP(Widget, char **);
extern void BX_SET_BACKGROUND_COLOR(Widget, ArgList, Cardinal *, Pixel);
extern void InitAppDefaults(Widget, UIAppDefault *);
extern void SetAppDefaults(Widget, UIAppDefault *, char *, Boolean);


// Begin user code block <head>
// End user code block <head>

//
// This table is used to define class resources that are placed
// in app-defaults. This table is necessary so each instance
// of this class has the proper default resource values set.
// This eliminates the need for each instance to have
// its own app-defaults values. This table must be NULL terminated.
//

Boolean RH_FileSB::_initAppDefaults = True;
UIAppDefault  RH_FileSB::_appDefaults[] = {

// Begin user code block <resource_table>
// End user code block <resource_table>
    {NULL, NULL, NULL, NULL, NULL}
};

// These are default resources for widgets in objects of this class
// All resources will be prepended by *<name> at instantiation,
// where <name> is the name of the specific instance, as well as the
// name of the baseWidget. These are only defaults, and may be overriden
// in a resource file by providing a more specific resource name

String  RH_FileSB::_defaultRH_FileSBResources[] = {
    
    // Begin user code block <default_resources>
    // End user code block <default_resources>
    NULL
};

//
// Class Constructor.
//
RH_FileSB::RH_FileSB(const char *name, Widget parent) : 
    UI_FileSB(name)

// Begin user code block <superclass>
// End user code block <superclass>
{
    
    // Begin user code block <constructor>
    // End user code block <constructor>
    
    create(parent);
    
    // Begin user code block <endconstructor>
    // End user code block <endconstructor>
}

//
// Alternate Constructor.
//
RH_FileSB::RH_FileSB(const char *name) : UI_FileSB(name)

// Begin user code block <alt_superclass>
// End user code block <alt_superclass>
{
    
    // Begin user code block <vk_alt_constructor>
    // End user code block <vk_alt_constructor>
}

//
// Minimal Destructor. Base class destroys widgets.
//
RH_FileSB::~RH_FileSB() 
{
    //
    // Class instances created in "create" method
    //
    
    
    // Begin user code block <destructor>
    // End user code block <destructor>
}

//
// Call superclass create to create common subclass widgets.
//
void RH_FileSB::create(Widget parent) 
{
    Boolean   argok = False;
    
    //
    // Load any class-defaulted resources for this object.
    //
    setDefaultResources(parent, _defaultRH_FileSBResources);
    //
    // Setup app-defaults fallback table if not already done.
    //
    if (_initAppDefaults)
    {
        InitAppDefaults(parent, _appDefaults);
        _initAppDefaults = False;
    }
    //
    // Now set the app-defaults for this instance.
    //
    SetAppDefaults(parent, _appDefaults, _name, False);
    
    UI_FileSB::create(parent);
    
    //
    // Override superclass resources here.
    //
    
    // Begin user code block <endcreate>
    // End user code block <endcreate>
}

//
// Classname access.
//
const char *RH_FileSB::className()
{
    return ("RH_FileSB");
}

// Begin user code block <tail>
void RH_FileSB::appendXcf ()
{
  _validExtensions.append ("xcf");
}

void RH_FileSB::appendFur ()
{
  _validExtensions.append ("fur");
}

void RH_FileSB::appendGbr ()
{
  _validExtensions.append ("gbr");
}
// End user code block <tail>
