//
// README: Portions of this file are merged at file generation
// time. Edits can be made *only* in between specified code blocks, look
// for keywords <Begin user code> and <End user code>.
//
//////////////////////////////////////////////////////////////
//
// Header file for RH_FileComboSB
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

#ifndef RH_FileComboSB_H
#define RH_FileComboSB_H
#include "UI_ComboSB.h"

//
// Globally included information (change thru Output File Names Dialog).
//


//
// Class Specific Includes (change thru the class in Resource Editor).
//



// Begin user code block <head>
#include <TWINE.h>
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

class RH_FileComboSB : public UI_ComboSB

// Begin user code block <superclass>
// End user code block <superclass>
{

// Begin user code block <friends>
// End user code block <friends>

  public:

    RH_FileComboSB(const char *, Widget);
    RH_FileComboSB(const char *);
    virtual ~RH_FileComboSB();
    const char *className();
    
    // Begin user code block <public>
    void appendXcf();  //this adds the xcf type to our extensions list 
    void appendFur();  //this adds the fur type to our extensions list 
    void appendGbr();  //this adds the gbr type to our extensions list 
    // End user code block <public>
  protected:
    virtual void create(Widget);
    
    // Widgets and Components created by RH_FileComboSB
    Widget _RH_FileComboSB;
    
    // These virtual functions are called from the private callbacks 
    // or event handlers intended to be overridden in derived classes
    // to define actions
    
    
    // Begin user code block <protected>
    void fileUpdateFilename();
    void ptsUpdateFilename();
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
    static String     _defaultRH_FileComboSBResources[];
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
