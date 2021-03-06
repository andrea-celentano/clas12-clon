
/* trigview.cxx - generates segment dictionary for the CLAS12 Drift Chamber */

#include "trigview.h"

#include "TROOT.h"
#include "TApplication.h"
#include "TGTableLayout.h"
#include "TGFrame.h"
#include "TList.h"
#include "TGWindow.h"
#include "TRootEmbeddedCanvas.h"
#include "TArc.h"
#include "TEllipse.h"
#include "TBox.h"
#include "TArrow.h"
#include "TPolyLine.h"
#include "TPolyMarker.h"
#include "TGButton.h"
#include "TCanvas.h"
#include "TRandom.h"
#include "TGDoubleSlider.h"
#include "TGaxis.h"
#include "Riostream.h"
#include "TText.h"


#include "evio.h"
#include "evioBankUtil.h"


#include "trigger.h"

#define USE_PCAL
#define FULLSIZE 800.

#include "ecalview.c"

static int handler; /* evio file handler */

int ecalview(int, TCanvas *fCanvas, int);

Viewer::Viewer(const TGWindow *win) : TGMainFrame(win,/*1120*/700,/*1144*/715)
{

  max_size = 300;

   // Memory management isn't the best in this class, but make a half
   // hearted attempts not to gush too much....
   fCleanup = new TList;

   TGCompositeFrame *table = new TGCompositeFrame(this,400,400,kSunkenFrame);
   fCleanup->Add(table);

   TGTableLayout* tlo = new TGTableLayout(table, 9, 9);

   table->SetLayoutManager(tlo);
   TGLayoutHints* loh = new TGLayoutHints(kLHintsTop|kLHintsLeft|
                                          kLHintsExpandX|kLHintsExpandY);
   AddFrame(table,loh);

   TGTableLayoutHints* tloh;

   //
   // The Canvas: main area for the drawing
   //
   TRootEmbeddedCanvas *recanvas =
       new TRootEmbeddedCanvas("CLAS12",table,max_size,max_size);

   //tloh = new TGTableLayoutHints(2,5,2,6,
   tloh = new TGTableLayoutHints(1,5,0,6,
                                 kLHintsExpandX|kLHintsExpandY |
                                 kLHintsShrinkX|kLHintsShrinkY |
                                 kLHintsFillX|kLHintsFillY);
   table->AddFrame(recanvas,tloh);
   fCanvas = recanvas->GetCanvas();


   printf("Viewer: calling fCanvas->Connect(MousePosition)\n");fflush(stdout);
   fCanvas->Connect("ProcessedEvent(Int_t,Int_t,Int_t,TObject*)",
       "Viewer", this,
       "MousePosition(Int_t, Int_t, Int_t, TObject*)");
   printf("Viewer: called fCanvas->Connect(MousePosition)\n");fflush(stdout);


   // Call this after scales/sliders are setup
   printf("===> %d %d\n",fCanvas->GetWw(),fCanvas->GetWh());

   SetRange(0.,0.,FULLSIZE,FULLSIZE);



   // buttons
   const char* shape_button_name[] = {
       "T0 -", "T0 +", "Fit", "Default View", "Zoom Out", "Zoom In",
       "Previous", "Next", "Close"
   };
   UInt_t ind;
   for (ind = 0; ind < 9; ++ind)
   {
      TGTextButton* button = new TGTextButton(table,shape_button_name[ind],ind);
      tloh = new TGTableLayoutHints(5,6,ind+0,ind+1+0,
                                    kLHintsExpandX|kLHintsExpandY |
                                    kLHintsShrinkX|kLHintsShrinkY |
                                    kLHintsFillX|kLHintsFillY);
      table->AddFrame(button,tloh);
      button->Resize(100,button->GetDefaultHeight());
      button->Connect("Clicked()","Viewer",this,"DoButton()");
   }


   
   // movements
   const char* ctrl_button_name[3][3] = {
      { "NW", "N", "NE" },
      { "W" , "C", "E" },
      { "SW", "S", "SE" }
   };
   UInt_t indx, indy;
   for (indx = 0; indx < 3; ++indx)
   for (indy = 0; indy < 3; ++indy)
   {
      TGTextButton* button = new TGTextButton(table,
                                              ctrl_button_name[indy][indx],
                                              (indx+1)*10 + indy +1);
      tloh = new TGTableLayoutHints(indx+2,indx+1+2, 6+indy,6+indy+1,
                                    kLHintsExpandX|kLHintsExpandY |
                                    kLHintsShrinkX|kLHintsShrinkY |
                                    kLHintsFillX|kLHintsFillY);
      table->AddFrame(button,tloh);
      button->Resize(100,button->GetDefaultHeight());
      button->Connect("Clicked()","Viewer",this,"DoButton()");
   }




   table->Layout();

   // exit on close window
   Connect("CloseWindow()","TApplication",gApplication,"Terminate(=0)");


   MapSubwindows();
   Layout();
   MapWindow();

   /*
   sxy = 0.015;
   x0 = 7.5;
   y0 = 7.5;
   fT0 = 6000.;
   xmin_xmax = 20.;
   */

   /*EEL*/
   /*
   sxy = 0.015;
   x0 = 2.5;
   y0 = 2.5;
   fT0 = 75.;
   xmin_xmax = 20.;
   */

   // real coordinates: can be changed by zooming
   x1 = 0;
   x2 = fCanvas->GetXsizeReal();
   y1 = 0;
   y2 = fCanvas->GetYsizeReal();
}


Viewer::~Viewer()
{
   fCanvas->Clear();
   delete fCanvas;
   fCleanup->Delete();
   delete fCleanup;
}



void Viewer::MousePosition(Int_t event, Int_t ix, Int_t iy, TObject *selected)
{
  Int_t ii, ixmin, iymin, ixmax, iymax, ixdif, iydif;

  /*printf("MousePosition reached\n");fflush(stdout);*/

  /*
  mouse events (see Buttons.h)
   kButton1Down - left button pressed
   kButton1Up   - left button released

   kButton2Down - middle button pressed
   kButton2Up   - middle button released

   kButton1Motion - motion with left button pressed
   kButton2Motion - motion with middle button pressed
   kMouseMotion - motion with no buttons pressed
   */


  if(event == kMouseEnter)
  {
    /*printf("MousePosition: event=%d, ix=%d, iy=%d, selected=0x%08x\n",event,ix,iy,selected);*/
    /*printf("ClassName >%s<\n",selected->ClassName());*/
	
    for(ii=0; ii<ndobj; ii++)
	{
      if(dobj[ii].obj == selected)
	  {
        printf("Object >%s< energy=%f\n",dobj[ii].name,dobj[ii].energy);
        return;
	  }
	}
    /*printf("Found nothing\n");*/

    return;
  }

  /*printf("MousePosition::: event=%d\n",event);*/

  fCanvas->FeedbackMode(kTRUE);

  // start drawing on kButton1Down, continue while kButton1Motion, end on kButton1Up
  if (event == kButton1Down)
  {
    /*printf("MousePosition: Start box drawing\n");*/
    xbox1 = ix;
    ybox1 = iy;
    xbox1old = 0;
  }
  else if (event == kButton1Motion)
  {
    /*printf("MousePosition: Continue box drawing\n");*/


    /*
         (xbox1,ybox1)   (xbox2,ybox1)

         (xbox1,ybox2)   (xbox2,ybox2)
     */

    // draw new lines
    xbox2 = ix;
    ybox2 = iy;


    // keep same ration between x and y
    ybox2 = ybox1 + (xbox2 - xbox1);

    /*printf("MousePosition: draw (%d,%d) -- (%d,%d)\n",xbox1,ybox1,xbox2,ybox2);*/
	
    gVirtualX->DrawLine (xbox1, ybox1, xbox2, ybox1);
    gVirtualX->DrawLine (xbox2, ybox1, xbox2, ybox2);
    gVirtualX->DrawLine (xbox2, ybox2, xbox1, ybox2);
    gVirtualX->DrawLine (xbox1, ybox2, xbox1, ybox1);
	

    xbox1old = xbox1;
    ybox1old = ybox1;
    xbox2old = xbox2;
    ybox2old = ybox2;

  }
  else if (event == kButton1Up)
  {
    /*printf("MousePosition: End box drawing: xbox1=%d ybox1=%d xbox2=%d ybox2=%d xdif=%d ydif=%d\n",
	   xbox1,ybox1,xbox2,ybox2,xbox2-xbox1,ybox2-ybox1);*/
    
    double xmin, ymin, xmax, ymax;
    double xdif, ydif;
    fCanvas->GetRange(xmin,ymin,xmax,ymax);
    xdif = xmax - xmin; ydif = ymax - ymin;
    /*printf("MousePosition: Float Range: xmin=%f ymin=%f xmax=%f ymax=%f xdif=%f ydif=%f\n",
	   xmin, ymin, xmax, ymax, xdif, ydif);*/

    ixmin = 0;
    ixmax = fCanvas->GetWw();
    iymin = fCanvas->GetWh();
    iymax = 0;
    ixdif = ixmax;
    iydif = iymin;
    /*printf("MousePosition: Int Range: ixmin=%d iymin=%d ixmax=%d iymax=%d ixdif=%d iydif=%d\n",
    	   ixmin, iymin, ixmax, iymax, ixdif, iydif);*/

    iydif = ixdif; // set iydif same as ixdif: keep same scale for both axis

    x1 = xmin + xbox1*(xdif/ixdif);
    y1 = ymax - ybox1*(ydif/iydif);
    x2 = xmin + xbox2*(xdif/ixdif);
    y2 = ymax - ybox2*(ydif/iydif);

    /*printf("MousePosition: selected area: x1=%f y1=%f x2=%f y2=%f\n",x1,y1,x2,y2);*/

    SetRange(x1, y2, x2, y1);

    printf("MousePosition: ecalview(%d, 0x%08x, %d)\n",0,fCanvas,0);
    ecalview(0, fCanvas, 0);

    fCanvas->Modified();
    fCanvas->Update();
  }

  return;
}



void
Viewer::DoButton()
{
  int redraw = 1;
  int first, inter, ifevb;

  ifevb = 1;
  first = 0;
  inter = 1;

   TGButton* button = (TGButton*)gTQSender;
   UInt_t id = button->WidgetId();

   double xmin, ymin, xmax, ymax;
   double xdif, ydif;
   fCanvas->GetRange(xmin,ymin,xmax,ymax);
   xdif = xmax - xmin; ydif = ymax - ymin;

   fCanvas->cd();
   /*fCanvas->Clear();*/

   switch (id) {
   case 0: {                   // T0 -
     fT0 = fT0 * 1.1;
     printf("T0-: set T0=%f\n",fT0);fflush(stdout);
     break;
   }
   case 1: {                   // T0 +
     fT0 = fT0 * 0.9;
     printf("T0+: set T0=%f\n",fT0);fflush(stdout);
     break;
   }
   case 2: {                   // Fit
      break;
   }
   case 3: {                   // Default view
     SetRange(0.,0.,FULLSIZE,FULLSIZE);
     fCanvas->GetRange(xmin,ymin,xmax,ymax);
     break;
   }
   case 4:                     // Zoom out
      SetRange(xmin-xdif*0.1, ymin-ydif*0.1, xmax+xdif*0.1, ymax+ydif*0.1);
      redraw=0;
      break;
   case 5:                     // Zoom in
      SetRange(xmin+xdif*0.1, ymin+ydif*0.1, xmax-xdif*0.1, ymax-ydif*0.1);
      redraw=0;
      break;
   case 6:
      printf("Previous\n");
      first = handler;
      {
        Double_t xmin, ymin, xmax, ymax;
        fCanvas->GetRange(xmin,ymin,xmax,ymax);
      }
      break;
   case 7:
	  printf("Next\n");fflush(stdout);
      first = handler;
      {
        Double_t xmin, ymin, xmax, ymax;
        fCanvas->GetRange(xmin,ymin,xmax,ymax);
      }
      break;
   case 8:
     printf("Close\n");
      gApplication->Terminate(0);
      break;
   case 11:                    // nw
      SetRange(xmin-xdif*.1, ymin+ydif*.1, xmax-xdif*.1, ymax+ydif*.1);
      redraw=0;
      break;
   case 12:                    // w
      SetRange(xmin-xdif*.1, ymin, xmax-xdif*.1, ymax);
      redraw=0;
      break;
   case 13:                    // sw
      SetRange(xmin-xdif*.1, ymin-ydif*.1, xmax-xdif*.1, ymax-ydif*.1);
      redraw=0;
      break;
   case 23:                    // s
      SetRange(xmin, ymin-ydif*.1, xmax, ymax-ydif*.1);
      redraw=0;
      break;
   case 33:                    // se
      SetRange(xmin+xdif*.1, ymin-ydif*.1, xmax+xdif*.1, ymax-ydif*.1);
      redraw=0;
      break;
   case 32:                    // e
      SetRange(xmin+xdif*.1, ymin, xmax+xdif*.1, ymax);
      redraw=0;
      break;
   case 31:                    // ne
      SetRange(xmin+xdif*.1, ymin+ydif*.1, xmax+xdif*.1, ymax+ydif*.1);
      redraw=0;
      break;
   case 21:                    // n
      SetRange(xmin, ymin+ydif*.1, xmax, ymax+ydif*.1);
      redraw=0;
      break;
   case 22:                    // c
      SetRange(50.0-xdif/2,50-ydif/2,50+xdif/2,50+ydif/2);
      redraw=0;
      break;
   } // end switch(id)



   printf("DoButton: ecalview((%d, 0x%08x, %d))\n",first,fCanvas,redraw);
   ecalview(first, fCanvas, redraw);

   fCanvas->Modified();
   fCanvas->Update();

}


void Viewer::SetRange(Float_t xmin, Float_t ymin, Float_t xmax, Float_t ymax, Bool_t move_slider)
{
  printf("\n\nAAAAAAAAAAAAAAAAAAAAAA SetRange: xmin=%f, xmax=%f, ymin=%f, ymax=%f\n",xmin, xmax, ymin, ymax);

   fCanvas->Range(xmin,ymin,xmax,ymax);

   fCanvas->Modified();
   fCanvas->Update();
   fClient->ForceRedraw();  //prevent GUI from being starved of redraws
}




int mynev;

//---- Main program ------------------------------------------------------------

int
main(int argc, char **argv)
{
  int status, iev;
  int batch = 0;

  printf("--> %d %s %s %s\n",argc,argv[0],argv[1],argv[2]);fflush(stdout);

  /******************/
  /* reconstruction */

  printf("argc=%d\n",argc);fflush(stdout);
  if(argc != 2 && argc != 3)
  {
    printf("Usage: trigview <evio_filename>\n");
    exit(1);
  }
  else if(argc==3 && !strcmp(argv[2],"batch"))
  {
    batch = 1;
    printf("running in batch mode\n");fflush(stdout);
  }

  status = evOpen(argv[1],"r",&handler);
  if(status < 0)
  {
    printf("evOpen error %d - exit\n",status);
    exit(0);
  }

  printf("running, data file >%s<\n",argv[1]);fflush(stdout);



  TApplication theApp("App", &argc, argv);
  
  /*
  if (gROOT->IsBatch())
  {
    fprintf(stderr, "%s: cannot run in batch mode\n", argv[0]);
    return(1);
  }
  */

  if(batch)
  {
    for(iev=0; iev<3500; iev++)
    {
      if(!(iev%10000)) printf("%d events processed\n",iev);
      status = ecalview(handler, 0, 0);
      if(status==EOF) break;
    }
  }
  else
  {
    Viewer viewer(gClient->GetRoot());
    theApp.Run();
  }

  status = evClose(handler);


  exit(0);
}


