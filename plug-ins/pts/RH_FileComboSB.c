//
// README: Portions of this file are merged at file generation
// time. Edits can be made *only* in between specified code blocks, look
// for keywords <Begin user code> and <End user code>.
//
/////////////////////////////////////////////////////////////
//
// Source file for RH_FileComboSB
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
#include "/software/thalia/h/pts.h"
#include <dm.h>

#include <ListTemplate.h>
#include <UI_TabbedDeck.h>
#include <UI_FileSB.h>
#include <UI_PtsFile.h>

#include "RH_FileSB.h"
// End user code block <head>

//
// This table is used to define class resources that are placed
// in app-defaults. This table is necessary so each instance
// of this class has the proper default resource values set.
// This eliminates the need for each instance to have
// its own app-defaults values. This table must be NULL terminated.
//

Boolean RH_FileComboSB::_initAppDefaults = True;
UIAppDefault  RH_FileComboSB::_appDefaults[] = {

// Begin user code block <resource_table>
// End user code block <resource_table>
    {NULL, NULL, NULL, NULL, NULL}
};

// These are default resources for widgets in objects of this class
// All resources will be prepended by *<name> at instantiation,
// where <name> is the name of the specific instance, as well as the
// name of the baseWidget. These are only defaults, and may be overriden
// in a resource file by providing a more specific resource name

String  RH_FileComboSB::_defaultRH_FileComboSBResources[] = {
    
    // Begin user code block <default_resources>
    // End user code block <default_resources>
    NULL
};

//
// Class Constructor.
//
RH_FileComboSB::RH_FileComboSB(const char *name, Widget parent) : 
    UI_ComboSB(name)

// Begin user code block <superclass>
// End user code block <superclass>
{
    
    // Begin user code block <constructor>
    _ui_FileSB = NULL;
    _ui_PtsSB = NULL;
    // End user code block <constructor>
    
    create(parent);
    
    // Begin user code block <endconstructor>
    // End user code block <endconstructor>
}

//
// Alternate Constructor.
//
RH_FileComboSB::RH_FileComboSB(const char *name) : UI_ComboSB(name)
// Begin user code block <alt_superclass>
// End user code block <alt_superclass>
{
    // Begin user code block <vk_alt_constructor>
    _ui_FileSB = NULL;
    _ui_PtsSB = NULL;
    // End user code block <vk_alt_constructor>
}

//
// Minimal Destructor. Base class destroys widgets.
//
RH_FileComboSB::~RH_FileComboSB() 
{
    //
    // Class instances created in "create" method
    //
    
    
    // Begin user code block <destructor>
    if ( _ui_FileSB != NULL ) {
	delete _ui_FileSB;
    }
    if ( _ui_PtsSB != NULL ) {
	delete _ui_PtsSB;
    }
    // End user code block <destructor>
}

//
// Call superclass create to create common subclass widgets.
//
void RH_FileComboSB::create(Widget parent) 
{
    Boolean   argok = False;
    
    //
    // Load any class-defaulted resources for this object.
    //
    setDefaultResources(parent, _defaultRH_FileComboSBResources);
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
    
    UI_ComboSB::create(parent);
    
    //
    // Override superclass resources here.
    //
    
    // Begin user code block <endcreate>
    SetAppDefaults(_deck->deckParent(), _appDefaults, "ui_FileSB", True);
    _ui_FileSB = new RH_FileSB("RH_FileSB",_deck->deckParent());
    _ui_FileSB->show();
    
    _deck->registerChild( _ui_FileSB, "File" );
    
    SetAppDefaults(_deck->deckParent(), _appDefaults, "ui_PtsFile", True);
    _ui_PtsSB = new UI_PtsFile("ui_PtsFile",_deck->deckParent());
    _ui_PtsSB->show();
    
    _deck->registerChild( _ui_PtsSB, "PTS" );
    
    XtVaSetValues( _deck->baseWidget(),
	XmNtopAttachment, XmATTACH_FORM,
	XmNbottomAttachment, XmATTACH_FORM,
	XmNleftAttachment, XmATTACH_FORM,
	XmNrightAttachment, XmATTACH_FORM,
	XmNtopOffset, 0,
	XmNbottomOffset, 0,
	XmNleftOffset, 0,
	XmNrightOffset, 0,
	NULL );

    // End user code block <endcreate>
}

//
// Classname access.
//
const char *RH_FileComboSB::className()
{
    return ("RH_FileComboSB");
}

// Begin user code block <tail>

void RH_FileComboSB::appendXcf() 
{
    //printf("RH_FileComboSB::appendXcf\n");
    ((RH_FileSB *)_ui_FileSB)->appendXcf();
}

void RH_FileComboSB::appendFur() 
{
    //printf("RH_FileComboSB::appendFur\n");
    ((RH_FileSB *)_ui_FileSB)->appendFur();
}

void RH_FileComboSB::appendGbr() 
{
    printf("RH_FileComboSB::appendGbr\n");
    ((RH_FileSB *)_ui_FileSB)->appendGbr();
}

void
RH_FileComboSB::fileUpdateFilename()
{
  _filename = _ui_FileSB->filename();
}

void
RH_FileComboSB::ptsUpdateFilename()
{
    
    List<Twine> names;
    List<FRANGE> ptsFranges;
    
    _filename = "";
    ptsFranges = _ui_PtsSB->franges();
    if ( ptsFranges.size() != 0 ) {
	if (U_FrameInFrange(((UI_PtsFile*)_ui_PtsSB)->frame(),
			    ptsFranges.size(), &ptsFranges[0])) {
	    names = DM_BuildFilenames( (const char*)_ui_PtsSB->ptsName(),
			    ((UI_PtsFile*)_ui_PtsSB)->frame() );
	    if ( names.size() != 0 )
		_filename = (Twine)(names[0]);
	}
    }
}
// End user code block <tail>
