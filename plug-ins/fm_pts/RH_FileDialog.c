//
// README: Portions of this file are merged at file generation
// time. Edits can be made *only* in between specified code blocks, look
// for keywords <Begin user code> and <End user code>.
//
/////////////////////////////////////////////////////////////
//
// Source file for RH_FileDialog
//
//    This class is a subclass of the ViewKit VkGenericDialog
//    class.  For more information on VkGenericDialog and
//    ViewKit dialogs in general, see the ViewKit Programmer's
//    Guide as well as the man pages for the classes.  In
//    particular, the documentation for VkDialogManager
//    contains information on how to manipulate dialogs in the
//    ViewKit framework.
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
#include <Vk/VkGenericDialog.h>
#include <Xm/Form.h>
#include "RH_FileDialog.h"
#include "RH_FileComboSB.h"

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
#include "fm_pts.h"
// End user code block <head>

//
// This table is used to define class resources that are placed
// in app-defaults. This table is necessary so each instance
// of this class has the proper default resource values set.
// This eliminates the need for each instance to have
// its own app-defaults values. This table must be NULL terminated.
//

Boolean RH_FileDialog::_initAppDefaults = True;
UIAppDefault  RH_FileDialog::_appDefaults[] = {

// Begin user code block <resource_table>
// End user code block <resource_table>
    {NULL, NULL, NULL, NULL, NULL}
};

// These are default resources for widgets in objects of this class
// All resources will be prepended by *<name> at instantiation,
// where <name> is the name of the specific instance, as well as the
// name of the baseWidget. These are only defaults, and may be overriden
// in a resource file by providing a more specific resource name

String  RH_FileDialog::_defaultRH_FileDialogResources[] = {
    
    // Begin user code block <default_resources>
    // End user code block <default_resources>
    NULL
};

//
// Class Constructor.
//
RH_FileDialog::RH_FileDialog(const char *name) : 
    VkGenericDialog(name)

// Begin user code block <superclass>
// End user code block <superclass>
{
    
    // Begin user code block <constructor>
    // End user code block <constructor>
    
    //
    // Initialize class instance data members to NULL
    // in case the destructor is called before the
    // instance is created.
    //
    _rH_FileComboSB = NULL;
    
}


//
// Minimal Destructor. Base class destroys widgets.
//
RH_FileDialog::~RH_FileDialog() 
{
    //
    // Class instances created in "create" method
    //
    if ( _rH_FileComboSB != NULL ) delete _rH_FileComboSB;
    
    
    // Begin user code block <destructor>
    // End user code block <destructor>
}

//
// Handle creation of all widgets in the class.
//
Widget RH_FileDialog::createDialog(Widget parent_base) 
{
    Boolean   argok = False;
    
    //
    // Create the basic dialog structure..
    //
    Widget parent = VkGenericDialog::createDialog(parent_base);
    
    //
    // Load any class-defaulted resources for this object.
    //
    setDefaultResources(parent, _defaultRH_FileDialogResources);
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
    
    //
    // Register the converters for the widgets.
    //
    RegisterBxConverters(theApplication->appContext());
    XtInitializeWidgetClass((WidgetClass)xmFormWidgetClass);
    
    
    SetAppDefaults(parent, _appDefaults, "rH_FileComboSB", True);
    _rH_FileComboSB = new RH_FileComboSB("rH_FileComboSB", parent);
    _rH_FileComboSB->show();
    
    //
    // Set exposed resources.
    //
    ac = 0;
    XtSetArg(args[ac], XmNx, 11); ac++;
    XtSetArg(args[ac], XmNy, 11); ac++;
    XtSetArg(args[ac], XmNwidth, 408); ac++;
    XtSetArg(args[ac], XmNheight, 461); ac++;
    XtSetValues(_rH_FileComboSB->baseWidget(), args, ac);
    
    
    //
    //  Take care of any remaining hard-coded resources.
    //
    
    // Begin user code block <endcreate>
    List<Twine> ourAssets;
    ourAssets.append("image");
    ourAssets.append("depth");
    _rH_FileComboSB->setAssetType (ourAssets);	
    _rH_FileComboSB->appendXcf ();	
    _rH_FileComboSB->appendFur ();	
    // End user code block <endcreate>
    
    //
    // Return the top of the dialog hierarchy..
    //
    return parent;
}

//
// This method is called each time the dialog is posted. The
// dialog Widget is the actual Motif dialog widget being displayed,
// which may vary from call to call as opposed to the object, which
// is the same (unless an application deliberately creates
// multiple instances).
//
Widget RH_FileDialog::prepost(const char *msg, XtCallbackProc okCB,
                    XtCallbackProc cancelCB, XtCallbackProc applyCB,
                    XtPointer clientData, const char *helpString,
                    VkSimpleWindow *parentWindow)
{
    Widget dialog = VkGenericDialog::prepost(msg,
                                     okCB, cancelCB, applyCB, clientData,
                                     helpString, parentWindow);
    
    // Begin user code block <prepost>
    // End user code block <prepost>
    
    return dialog;
}

//
// Classname access.
//
const char *RH_FileDialog::className()
{
    return ("RH_FileDialog");
}

//
// The following methods are called when one of the buttons
// buttons of the dialog are pressed.
//
void RH_FileDialog::apply(Widget w, XtPointer callData)
{
    
    // Begin user code block <apply>
      //printf("RH_FileDialogSB:apply called\n");
      Twine rawFilename = this->rawFilename();
      Twine filename = this->filename();

      //printf("rawFilename is %s\n", (const char*)rawFilename);
      //printf("filename is %s\n", (const char*)filename);

      //printf("call file_load here\n"); 

      file_load ((const char*)rawFilename);

      VkGenericDialog::apply(w, callData);
    
    // End user code block <apply>
}

void RH_FileDialog::cancel(Widget w, XtPointer callData)
{
    
    // Begin user code block <cancel>
    
    // By default, call the superclass' cancel method
    VkGenericDialog::cancel(w, callData);
    
    // End user code block <cancel>
}

void RH_FileDialog::ok(Widget w, XtPointer callData)
{
    
    // Begin user code block <ok>

    int failed = FALSE;
    //printf("RH_FileDialog:ok called\n");
    Twine rawFilename = this->rawFilename();
    Twine filename = this->filename();

    //printf("rawFilename is %s\n", (const char*)rawFilename);
    //printf("filename is %s\n", (const char*)filename);

    
#if 0
    if (_mode == 0)
      printf("call file_load here\n"); 
    else if (_mode == 1 ) 
      printf("call file_save here\n"); 
    else if (_mode == 2 ) 
      printf("call file_save as here\n"); 
#endif

    if (_mode == 1 || _mode == 2)
      failed = file_save ((const char*)rawFilename, TRUE);
    else if (_mode == 0)
      failed = file_load ((const char*)rawFilename);

    if (!failed)
      VkGenericDialog::ok(w, callData);
  
    
    // End user code block <ok>
}

// Begin user code block <tail>
       
Twine RH_FileDialog::filename( void )
{
   //printf("RH_FileDialog::filename\n");
   return _rH_FileComboSB->filename();
}

Twine RH_FileDialog::rawFilename( void )
{
   //printf("RH_FileDialog::rawFilename\n");
   return _rH_FileComboSB->rawFilename();
}

int RH_FileDialog::getMode()
{
  return _mode;
}

void RH_FileDialog::setMode(int mode)
{
 _mode = mode;
}
	
// End user code block <tail>
