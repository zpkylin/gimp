//
// README: Portions of this file are merged at file generation
// time. Edits can be made *only* in between specified code blocks, look
// for keywords <Begin user code> and <End user code>.
//
//////////////////////////////////////////////////////////////
//
// Header file for RH_FileDialog
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
//////////////////////////////////////////////////////////////


// Begin user code block <file_comments>
// End user code block <file_comments>

#ifndef RH_FileDialog_H
#define RH_FileDialog_H
#include <Vk/VkGenericDialog.h>
class RH_FileComboSB;

//
// Globally included information (change thru Output File Names Dialog).
//


//
// Class Specific Includes (change thru the class in Resource Editor).
//



// Begin user code block <head>
class Twine;
// End user code block <head>

#ifndef VKDEFINES 
#define VKDEFINES 
typedef struct _UIAppDefault {
    char*      cName;       // Class name 
    char*      wName;       // Widget name 
    char*      cInstName;   // Name of class instance (nested class) 
    char*      wRsc;        // Widget resource 
    char*      value;       // value read from app-defaults 
} UIAppDefault;

#endif

class RH_FileDialog : public VkGenericDialog

// Begin user code block <superclass>
// End user code block <superclass>
{

// Begin user code block <friends>
// End user code block <friends>

  public:

    RH_FileDialog(const char *);
    virtual ~RH_FileDialog();
    const char *className();
    
    // Begin user code block <public>
    Twine filename();    //a (pts valid) filename from regular text field or pts file name.
    Twine rawFilename(); //the (raw) filename from regular text field or pts file name.
    void  setMode (int mode);
    int  getMode ();
    // End user code block <public>
  protected:
    virtual Widget createDialog(Widget);
    Widget prepost(const char *, XtCallbackProc, XtCallbackProc,
                   XtCallbackProc, XtPointer, const char *, VkSimpleWindow *);
    
    // Widgets and Components created by RH_FileDialog
    RH_FileComboSB *_rH_FileComboSB;
    
    // These virtual functions are called from the private callbacks 
    // or event handlers intended to be overridden in derived classes
    // to define actions
    
    virtual void apply(Widget, XtPointer);
    virtual void cancel(Widget, XtPointer);
    virtual void ok(Widget, XtPointer);
    
    // Begin user code block <protected>
    int _mode;
    // End user code block <protected>
  private: 
    
    //
    // Useful declarations for setting Xt Resources.
    //
    Cardinal          ac;
    Arg               args[256];
    
    //
    // Default application and class resources.
    //
    static String     _defaultRH_FileDialogResources[];
    static UIAppDefault   _appDefaults[];
    static Boolean        _initAppDefaults;
    
    //
    // ViewKit Static Menu Structures.
    //
    
    //
    // Callbacks to interface with Motif.
    //
    
    // Begin user code block <private>
    // End user code block <private>
};

// Begin user code block <tail>
// End user code block <tail>
#endif
