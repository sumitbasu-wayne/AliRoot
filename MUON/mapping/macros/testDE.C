// $Id$
// $MpId: testDE.C,v 1.1 2006/03/14 09:06:54 ivana Exp $
//
// Test AliMpDEIterator & AliMpSegFactory classes

#if !defined(__CINT__) || defined(__MAKECINT__)

#include "AliMpDEIterator.h"
#include "AliMpSegmentation.h"
#include "AliMpVSegmentation.h"
#include "AliMpCDB.h"
#include "AliMpIntPair.h"
#include "AliMpPad.h"

#include <Riostream.h>

#endif

void testDE() 
{
  AliMpCDB::LoadMpSegmentation2("local://$ALIROOT_OCDB_ROOT/OCDB");

  AliMpDEIterator it;
  for ( it.First(); ! it.IsDone(); it.Next() ) {
    cout << "In detection element: " << it.CurrentDEId() << endl;

    // Create/get segmentation via factory
    const AliMpVSegmentation* kSegmentation 
      = AliMpSegmentation::Instance()
          ->GetMpSegmentation(it.CurrentDEId(), AliMp::kCath0);
      
    // Print number of pads
   cout << "   number of pads: " << kSegmentation->NofPads() << endl;   
      
    // Find pad by indices in this DE
    Int_t ix = kSegmentation->MaxPadIndexX()/2;
    Int_t iy = kSegmentation->MaxPadIndexY()/2;
    AliMpPad pad = kSegmentation->PadByIndices(ix, iy, false);
    
    cout << "   found pad: " << pad << endl << endl;
  }
}
