//
// README: Portions of this file are merged at file generation
// time. Edits can be made *only* in between specified code blocks, look
// for keywords <Begin user code> and <End user code>.
//
//////////////////////////////////////////////////////////////////////
//
// This driver program was created with:
//
//    Builder Xcessory Version 5.0.5
//    Code Generator Xcessory 5.0.2 (03/15/99) Script Version 5.0.5
//
//////////////////////////////////////////////////////////////////////


// Begin user code block <file_comments>
// End user code block <file_comments>

//
// ViewKit, Motif, and X Required Includes.
//
#include <X11/StringDefs.h>
#include <Xm/Xm.h>
#include <Xm/DialogS.h>
#include <Xm/MwmUtil.h>
#include <Xm/RepType.h>
#include <Vk/VkApp.h>
#include <Vk/VkSimpleWindow.h>

//
// Globally included information (change thru Output File Names Dialog).
//


//
// Headers for classes used in this program.
//
#include "RH_FileDialog.h"
#include "RH_FileSB.h"
#include <UI_FileSB.h>
#include "RH_FileComboSB.h"
#include <UI_ComboSB.h>

//
// Convenience functions from utilities file.
//
extern void RegisterBxConverters(XtAppContext);
extern XtPointer BX_CONVERT(Widget, char *, char *, int, Boolean *);
extern XtPointer BX_DOUBLE(double);
extern XtPointer BX_SINGLE(float);
extern void BX_MENU_POST(Widget, XtPointer, XEvent *, Boolean *);
extern Pixmap XPM_PIXMAP(Widget, char **);
extern void BX_SET_BACKGROUND_COLOR(Widget, ArgList, Cardinal *, Pixel);

// Begin user code block <globals>
#include <Vk/VkWarningDialog.h>
#include <Vk/VkQuestionDialog.h>
#include "main-vk.h"
// End user code block <globals>
//
// Change this line via the Output Application Names Dialog.
//
#define BX_APP_CLASS "BuilderProduct"

int vk_main ( int mode )
{
    Cardinal ac;
    Arg      args[256];
    VkApp    *app;
    
    // Begin user code block <declarations>

    int argc = 1;                                                                        
    char **argv;                                                                         
    char *load_str = {"Open"};                                                     
    char *save_str = {"Save"};     
    char *save_as_str = {"Save As"};     
    char *set_dir_str = {"Set Dir"};     
	

    switch (mode)
    {
      case 0:
	argv = &load_str;
	break;
      case 1:
	argv = &save_str;
	break;
      case 2:
	argv = &save_as_str;
	break;
      case 3:
      case 4:
	argv = &set_dir_str;
	break;
      default:
	return 1;
    }

    // End user code block <declarations>
    
    //
    // Initialize the Fallback Resources.
    //
    char     *fallbackResources[] = {
        
        // Begin user code block <fallbacks>
        NULL
        // End user code block <fallbacks>
    
    };
    VkApp::setFallbacks(fallbackResources);
    
    // Begin user code block <app_class_args>
    XrmOptionDescRec *optionList = NULL;
    int              numOptions = 0;
    // End user code block <app_class_args>
    
    app = new VkApp (BX_APP_CLASS, &argc, argv,
                    optionList, numOptions);
    RegisterBxConverters(app->appContext());
    
    // Begin user code block <create_shells>
    // End user code block <create_shells>
    
    //
    // Instantiate window classes used in this program.
    //
    
    Boolean   argok = False;
    
    
    // Begin user code block <create_rH_FileDialog>
    // End user code block <create_rH_FileDialog>
    
    RH_FileDialog *_rH_FileDialog = new RH_FileDialog("rH_FileDialog");
    _rH_FileDialog->setMode (mode);                                                            

    if (mode == 0) 
       _rH_FileDialog->postAndWait(NULL, TRUE, TRUE, TRUE); 
     else if (mode == 1 || mode == 2) 
       _rH_FileDialog->postAndWait(NULL, TRUE, TRUE, FALSE);
    else if (mode == 3 || mode == 4)
      {
     _rH_FileDialog->postAndWait(NULL, TRUE, TRUE, FALSE);
      }
    // 
    // Set exposed resources.  
    // 
    // Begin user code block <app_procedures>
    // End user code block <app_procedures>
    
    // Begin user code block <main_loop>
	//We dont need an app->run since we are just a dialog.
    // End user code block <main_loop>
    //app->run();
    
    //
    // A return value even though the event loop never ends.
    //
    return(0);
}

void doWarningDialog(const char *warning)                                                
{                                                                                        
   theWarningDialog->postAndWait(warning);                                               
}                                                                                        
                                                                                         
int doOverwrite(const char *question)                                                    
{                                                                                        
   if (theQuestionDialog->postAndWait(question) == VkDialogManager::OK)                  
        return TRUE;                                                                     
   else                                                                                  
        return FALSE;                                                                    
}  





