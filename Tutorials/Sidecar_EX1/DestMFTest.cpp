// --------------------------------------------------------------------------
//   DestMFTest.cpp
// --------------------------------------------------------------------------
// An example to test copying data from one fabarray to another
//   in another group
// --------------------------------------------------------------------------
#include <new>
#include <iostream>
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <sstream>
#include <unistd.h>

#include <BoxLib.H>
#include <ParallelDescriptor.H>
#include <Utility.H>
#include <ParmParse.H>
#include <MultiFab.H>
#include <VisMF.H>


int nComp(6), nGhost(2);


// --------------------------------------------------------------------------
namespace
{
  const int S_SendBoxArray(42), S_CFATests(43), S_CopyFabArray(44);


  // --------------------------------------------------------------------------
  void CopyFabArray(MultiFab *mfSource, MultiFab *mfDest) {

    static int count(0);
    std::stringstream css;
    css << "TS_" << count << "_";

    VisMF::SetNOutFiles(1);
    if(mfSource != 0) {
      VisMF::Write(*mfSource, css.str() + "mfSource_before");
    }
    if(mfDest != 0) {
      VisMF::Write(*mfDest, css.str() + "mfDest_before");
    }


    MultiFab::copyInter(mfSource, mfDest, 0, 0, nComp, 0, 0,
                        ParallelDescriptor::CommunicatorComp(),
                        ParallelDescriptor::CommunicatorSidecar(),
                        ParallelDescriptor::CommunicatorInter(),
                        ParallelDescriptor::InCompGroup());

    if(mfSource != 0) {
      VisMF::Write(*mfSource, css.str() + "mfSource_after");
    }
    if(mfDest != 0) {
      VisMF::Write(*mfDest, css.str() + "mfDest_after");
    }
    ++count;
  }




  // --------------------------------------------------------------------------
  void CFATests(BoxArray &ba, DistributionMapping &dm) {

    int myProcAll(ParallelDescriptor::MyProcAll());
    int myProcComp(ParallelDescriptor::MyProcComp());
    int myProcSidecar(ParallelDescriptor::MyProcSidecar());
    bool addToCache(false);
    MPI_Group group_sidecar(MPI_GROUP_NULL), group_comp(MPI_GROUP_NULL), group_all(MPI_GROUP_NULL);

    ParallelDescriptor::Barrier(ParallelDescriptor::CommunicatorAll());
    BoxLib::USleep(myProcAll / 10.0);
    std::cout << ":::: _in CFATests:  myProcAll myProcComp myProcSidecar = " << myProcAll
              << "  " << myProcComp << "  " << myProcSidecar  << std::endl;

    Array<int> pm_sidecar, pm_comp, pm_sidecar_all, pm_comp_all;
    DistributionMapping dm_comp_all, dm_sidecar_all;
    BL_MPI_REQUIRE( MPI_Comm_group(ParallelDescriptor::CommunicatorAll(), &group_all) );

    if(ParallelDescriptor::InSidecarGroup()) {
      MPI_Comm_group(ParallelDescriptor::CommunicatorSidecar(), &group_sidecar);
      pm_sidecar = dm.ProcessorMap();
      pm_sidecar_all = DistributionMapping::TranslateProcMap(pm_sidecar, group_all, group_sidecar);
      // Don't forget to set the sentinel to the proc # in the new group!
      pm_sidecar_all[pm_sidecar_all.size()-1] = ParallelDescriptor::MyProcAll();

      dm_sidecar_all.define(pm_sidecar_all, addToCache);
      if(ParallelDescriptor::IOProcessor()) {
        std::cout << myProcAll << ":::: _in CFATests:  dm = " << dm << std::endl;
        std::cout << myProcAll << ":::: _in CFATests:  dm_sidecar_all = " << dm_sidecar_all << std::endl;
      }
    }

      dm_comp_all.define(pm_comp_all, addToCache);
      BoxLib::USleep(myProcAll / 10.0);
      if(myProcAll == 0) {
        std::cout << myProcAll << ":::: _in CFATests:  dm_comp = " << dm << std::endl;
      }
  }


  // --------------------------------------------------------------------------
  void SidecarEventLoop() {
    bool finished(false);
    int sidecarSignal(-1), time_step(-2);
    int myProcAll(ParallelDescriptor::MyProcAll());
    BoxArray bac, bab;
    DistributionMapping sc_DM;

    while ( ! finished) {  // ---- Receive the signal from the compute group.

        ParallelDescriptor::Bcast(&sidecarSignal, 1, 0, ParallelDescriptor::CommunicatorInter());

	switch(sidecarSignal) {

	  case S_SendBoxArray:
          {
	    bac.clear();
	    BoxArray::RecvBoxArray(bac);
            if(ParallelDescriptor::IOProcessor()) {
              std::cout << myProcAll << ":: sidecar recv ba.size = " << bac.size() << std::endl;
	    }

            MPI_Group group_sidecar, group_all;
            MPI_Comm_group(ParallelDescriptor::CommunicatorSidecar(), &group_sidecar);
            MPI_Comm_group(ParallelDescriptor::CommunicatorAll(), &group_all);

            // Create DM on sidecars with default strategy
            const DistributionMapping dm_sidecar(bac, ParallelDescriptor::NProcsSidecar());
            const Array<int> pm_sidecar = dm_sidecar.ProcessorMap();

            Array<int> pm_all = DistributionMapping::TranslateProcMap(pm_sidecar, group_all, group_sidecar);
            // Don't forget to set the sentinel to the proc # in the new group!
            pm_all[pm_all.size()-1] = ParallelDescriptor::MyProcAll();

            DistributionMapping dm_all(pm_all);
            if (ParallelDescriptor::IOProcessor()) {
              BoxLib::USleep(1);
              std::cout << "SIDECAR DM = " << dm_sidecar << std::endl << std::flush;
              std::cout << "WORLD DM = " << dm_all << std::endl << std::flush;
            }
          }
	  break;

	  case S_CopyFabArray:
	  {
            if(ParallelDescriptor::IOProcessor()) {
              std::cout << "Sidecars received the S_CopyFabArray signal." << std::endl;
	    }
	    bac.clear();
	    BoxArray::RecvBoxArray(bac);
            if(ParallelDescriptor::IOProcessor()) {
              std::cout << myProcAll << ":: sidecar recv ba.size = " << bac.size() << std::endl;
	    }
	    sc_DM.define(bac, ParallelDescriptor::NProcsSidecar());
            MultiFab mf(bac, nComp, nGhost, sc_DM, Fab_allocate);
	    mf.setVal(-8.91);
	    MultiFab *mfSource = 0;
	    MultiFab *mfDest = &mf;
            CopyFabArray(mfSource, mfDest);
          }
	  break;

	  case S_CFATests:
	  {
            if(ParallelDescriptor::IOProcessor()) {
              std::cout << "Sidecars received the S_CFATests signal." << std::endl;
	    }
	    sc_DM.define(bac, ParallelDescriptor::NProcsSidecar());
            CFATests(bac, sc_DM);
          }
	  break;

	  case ParallelDescriptor::SidecarQuitSignal:
	  {
            if(ParallelDescriptor::IOProcessor()) {
              std::cout << "Sidecars received the quit signal." << std::endl;
	    }
            finished = true;
	  }
	  break;

	  default:
	  {
            if(ParallelDescriptor::IOProcessor()) {
              std::cout << "**** Sidecars received bad signal = " << sidecarSignal << std::endl;
	    }
	  }
	  break;
	}

    }
    if(ParallelDescriptor::IOProcessor()) {
      std::cout << "===== SIDECARS DONE. EXITING ... =====" << std::endl;
    }
  }

}



// --------------------------------------------------------------------------
int main(int argc, char *argv[]) {

    BoxLib::Initialize(argc,argv);

    // A flag you need for broadcasting across MPI groups. We always broadcast
    // the data to the sidecar group from the IOProcessor on the compute group.
    int MPI_IntraGroup_Broadcast_Rank;
    int myProcAll(ParallelDescriptor::MyProcAll());
    int nSidecarProcs(0), sidecarSignal(S_SendBoxArray);
    int maxGrid(32), maxSize(16);
    int ts(0), nSteps(5);
    ParmParse pp;

    pp.query("nSidecars", nSidecarProcs);
    pp.query("maxGrid", maxGrid);
    pp.query("nComp", nComp);
    pp.query("nGhost", nGhost);
    pp.query("maxSize", maxSize);
    pp.query("nSteps", nSteps);

    if(ParallelDescriptor::IOProcessor()) {
      std::cout << "nSidecarProcs = " << nSidecarProcs << std::endl;
    }

    if(ParallelDescriptor::IOProcessor()) {
      std::cout << myProcAll << ":: Resizing sidecars = " << nSidecarProcs << std::endl;
    }
    ParallelDescriptor::SetNProcsSidecar(nSidecarProcs);
    MPI_IntraGroup_Broadcast_Rank = ParallelDescriptor::IOProcessor() ? MPI_ROOT : MPI_PROC_NULL;


    if(ParallelDescriptor::InSidecarGroup()) {

      BoxLib::USleep(myProcAll / 10.0);
      std::cout << myProcAll << ":: Calling SidecarEventLoop()." << std::endl;
      SidecarEventLoop();

    } else {

      // ---- Build a Box to span the problem domain, and split it into a BoxArray
      Box baseBox(IntVect(0,0,0), IntVect(maxGrid-1, maxGrid-1, maxGrid-1));
      BoxArray ba(baseBox);
      ba.maxSize(maxSize);

      // ---- This is the DM for the compute processes.
      DistributionMapping comp_DM;
      comp_DM.define(ba, ParallelDescriptor::NProcsComp());


      for(int i(ts); i < ts + nSteps; ++i) {  // ----- do time steps.
	if(ParallelDescriptor::IOProcessor()) {
	  std::cout << myProcAll << ":: Doing timestep = " << i << std::endl;
	}

	if(nSidecarProcs > 0) {

	  if((i - ts) == 0) {  // ---- do a simple mf copy test
	    MultiFab mf(ba, nComp, nGhost, comp_DM, Fab_allocate);
	    for(MFIter mfi(mf); mfi.isValid(); ++mfi) {
	      for(int i(0); i < mf[mfi].nComp(); ++i) {
	        mf[mfi].setVal(myProcAll + (Real) i / 1000.0, i);
	      }
	    }

	    sidecarSignal = S_CopyFabArray;
	    ParallelDescriptor::Bcast(&sidecarSignal, 1, MPI_IntraGroup_Broadcast_Rank,
	                              ParallelDescriptor::CommunicatorInter());
	    BoxArray::SendBoxArray(ba);

	    MultiFab *mfSource = &mf;
	    MultiFab *mfDest = 0;
            CopyFabArray(mfSource, mfDest);
	  }

	  if((i - ts) == 1) {  // ---- do a shrinked boxarray mf copy test
	    BoxArray bashrink(ba);
	    bashrink.grow(-1);
	    MultiFab mf(bashrink, nComp, nGhost, comp_DM, Fab_allocate);
	    for(MFIter mfi(mf); mfi.isValid(); ++mfi) {
	      for(int i(0); i < mf[mfi].nComp(); ++i) {
	        mf[mfi].setVal(myProcAll + (Real) i / 1000.0, i);
	      }
	    }

	    sidecarSignal = S_CopyFabArray;
	    ParallelDescriptor::Bcast(&sidecarSignal, 1, MPI_IntraGroup_Broadcast_Rank,
	                              ParallelDescriptor::CommunicatorInter());
	    BoxArray::SendBoxArray(ba);  // ---- send the sidecar the unshrunk boxarray

	    MultiFab *mfSource = &mf;
	    MultiFab *mfDest = 0;
            CopyFabArray(mfSource, mfDest);
	  }


	  if((i - ts) == 2) {  // ---- do a shifted boxarray mf copy test
	    BoxArray bashift(ba);
	    bashift.shift(IntVect(1,2,3));
	    MultiFab mf(bashift, nComp, nGhost, comp_DM, Fab_allocate);
	    for(MFIter mfi(mf); mfi.isValid(); ++mfi) {
	      for(int i(0); i < mf[mfi].nComp(); ++i) {
	        mf[mfi].setVal(myProcAll + (Real) i / 1000.0, i);
	      }
	    }

	    sidecarSignal = S_CopyFabArray;
	    ParallelDescriptor::Bcast(&sidecarSignal, 1, MPI_IntraGroup_Broadcast_Rank,
	                              ParallelDescriptor::CommunicatorInter());
	    BoxArray::SendBoxArray(ba);  // ---- send the sidecar the unshifted boxarray

	    MultiFab *mfSource = &mf;
	    MultiFab *mfDest = 0;
            CopyFabArray(mfSource, mfDest);
	  }



	  //sidecarSignal = S_SendBoxArray;
	  //ParallelDescriptor::Bcast(&sidecarSignal, 1, MPI_IntraGroup_Broadcast_Rank,
	                            //ParallelDescriptor::CommunicatorInter());
	  //BoxArray::SendBoxArray(ba);

	  //sidecarSignal = S_CFATests;
	  //ParallelDescriptor::Bcast(&sidecarSignal, 1, MPI_IntraGroup_Broadcast_Rank,
	                            //ParallelDescriptor::CommunicatorInter());
          //CFATests(ba, comp_DM);

	}
      }
      ts += nSteps;

      if(nSidecarProcs > 0) {
	// ---- stop the sidecars
	sidecarSignal = ParallelDescriptor::SidecarQuitSignal;
	ParallelDescriptor::Bcast(&sidecarSignal, 1, MPI_IntraGroup_Broadcast_Rank,
	                          ParallelDescriptor::CommunicatorInter());
      }
    }
    
    ParallelDescriptor::Barrier();
    BoxLib::USleep(myProcAll / 10.0);
    if(ParallelDescriptor::IOProcessor()) {
      std::cout << myProcAll << ":: Finished timesteps" << std::endl;
    }

    ParallelDescriptor::Barrier();
    nSidecarProcs = 0;
    ParallelDescriptor::SetNProcsSidecar(nSidecarProcs);


    BoxLib::Finalize();
    return 0;
}
// --------------------------------------------------------------------------
// --------------------------------------------------------------------------