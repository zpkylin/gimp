//
// README: Portions of this file are merged at file generation
// time. Edits can be made *only* in between specified code blocks, look
// for keywords <Begin user code> and <End user code>.
//
//////////////////////////////////////////////////////////////
//
// Header file for RH_FileSB
//
//    This class is a ViewKit "component", as described
//    in the ViewKit Programmer's Guide.
//
// This file created with:
//
//    Builder Xcessory Version 5.0.5
//    Code Generator Xcessory 5.0.2 (03/15/99) Script Version 5.0.5
//
//////////////////////////////////////////////////////////////


// Begin user code block <file_comments>
// End user code block <file_comments>

#ifndef RH_FileSB_H
#define RH_FileSB_H
#include <UI_FileSB.h>

//
// Globally included information (change thru Output File Names Dialog).
//


//
// Class Specific Includes (change thru the class in Resource Editor).
//



// Begin user code block <head>
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

class RH_FileSB : public UI_FileSB

// Begin user code block <superclass>
// End user code block <superclass>
{

// Begin user code block <friends>
// End user code block <friends>

  public:

    RH_FileSB(const char *, Widget);
    RH_FileSB(const char *);
    virtual ~RH_FileSB();
    const char *className();
    
    // Begin user code block <public>
    void appendXcf ();
    void appendFur ();
    void appendGbr ();
    // End user code block <public>
  protected:
    virtual void create(Widget);
    
    // Widgets and Components created by RH_FileSB
    Widget _RH_FileSB;
    
    // These virtual functions are called from the private callbacks 
    // or event handlers intended to be overridden in derived classes
    // to define actions
    
    
    // Begin user code block <protected>
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
    static String     _defaultRH_FileSBResources[];
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
