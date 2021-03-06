
#undef  BL_LANG_CC
#ifndef BL_LANG_FORT
#define BL_LANG_FORT
#endif

#include <ArrayLim.H>

#define SDIM 1

c ::: -----------------------------------------------------------
c ::: This routine sets the values for the lo() and hi() arrays
c ::: from the ARG_L1, ARG_H1, ... macros.  This is done since
c ::: it is more convenient to use the lo() and hi() arrays.
c :::
c ::: INPUTS/OUTPUTS:
c :::
c ::: DIMS(holder)=> index extent of place holder array
c ::: lo(SDIM)    <= lower index limits
c ::: hi(SDIM)    <= upper index limits
c ::: -----------------------------------------------------------

      subroutine SET_LOHI(DIMS(holder), lo, hi)

      implicit none

c
c     :::: Passed Variables ::::
c
      integer DIMDEC(holder)
      integer lo(SDIM), hi(SDIM)


c
c     --------------------------------------
c     :::: Set Values for lo() and hi() ::::
c     --------------------------------------
c
      lo(1) = ARG_L1(holder)
      hi(1) = ARG_H1(holder)

      end


c ::: -----------------------------------------------------------
c ::: This routine sets the values for the ARG_L1, ARG_H1, ... macros
c ::: from the lo() and hi() arrays.  This is done since
c ::: it is more convenient to use the macros to dimension arrays.
c :::
c ::: INPUTS/OUTPUTS:
c :::
c ::: FF_DIMS(holder) <=  index extent of place holder array
c ::: lo(SDIM)         => lower index limits
c ::: hi(SDIM)         => upper index limits
c ::: -----------------------------------------------------------

      subroutine SET_ARGS(DIMS(holder), lo, hi)

      implicit none

c
c     :::: Passed Variables ::::
c
      integer DIMDEC(holder)
      integer lo(SDIM), hi(SDIM)

c
c     --------------------------------------
c     :::: Set Values for lo() and hi() ::::
c     --------------------------------------
c
      ARG_L1(holder) = lo(1)
      ARG_H1(holder) = hi(1)

c
c
      return
      end

