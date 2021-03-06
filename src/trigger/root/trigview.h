
/* trigview.h */

#ifndef ROOT_TGFrame
#include "TGFrame.h"
#endif

class TList;
class TCanvas;
class TRootEmbeddedCanvas;
class TGaxis;
class TGDoubleSlider;


class Viewer : public TGMainFrame {

private:
   TList               *fCleanup;
   TCanvas             *fCanvas;
   TRootEmbeddedCanvas *fHScaleCanvas, *fVScaleCanvas;
   TGaxis              *fHScale, *fVScale;
   TGDoubleSlider      *fHSlider;
   TGDoubleSlider      *fVSlider;

   UInt_t               max_size;


   Int_t                xbox1, ybox1, xbox2, ybox2;
   Int_t                xbox1old, ybox1old, xbox2old, ybox2old;

      Float_t sxy;
      Float_t x0;
      Float_t y0;
      Float_t fT0;
      Float_t xmin_xmax;

  Float_t x1,y1,x2,y2;


   //cmon
   Int_t bcs_iw[1000000];
   //cmon

public:
   Viewer(const TGWindow *win);
   virtual ~Viewer();

   void MousePosition(Int_t event, Int_t x, Int_t y, TObject *selected);

   void     DoButton();
   TCanvas *GetCanvas() {return(fCanvas);}
   void     SetRange(Float_t xmin, Float_t ymin, Float_t xmax, Float_t ymax,
                 Bool_t move_slider = kTRUE);
   ClassDef(Viewer,0) //GUI example
};
