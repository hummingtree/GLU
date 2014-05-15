/*
    Copyright 2013 Renwick James Hudspith

    This file (MAG.c) is part of GLU.

    GLU is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    GLU is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with GLU.  If not, see <http://www.gnu.org/licenses/>.
*/
/**
   @file MAG.c
   @brief Maximal Axial Gauge fixing

  Here I have included the Maximal Axial Gauge fixing and
  the \f$ A^{\mu} = 0 \f$ partial axial gauge fixing.

  MAG performs a lattice-wide reunitarisation step first.

  TODO :: Implicitly do the rotation and set to identity
 */

#include "Mainfile.h"

#include "geometry.h"      // get site 
#include "givens.h"        // residual fixing uses this
#include "gtrans.h"        // lattice-wide gauge transformations
#include "gramschmidt.h"   // orthogonalisation
#include "random_config.h" // lattice reunitarisation and stuff

//calculate gauge transform matrices along one line in one direction
static void 
gauge_set( lat , mu , i , gauge )
     struct site *__restrict lat ; 
     GLU_complex *__restrict *__restrict gauge ; 
     const int i  ,  mu ; 
{  
  int left = i ; 
  int next = lat[i].neighbor[mu] ; 
  // set origin to identity
  identity( gauge[i] ) ; 
  //loop along the lattice
  int j ; 
  for( j = 0 ; j < ( Latt.dims[ mu ]-1 ) ; j++ ) {
    multab_suNC( gauge[ next ] , gauge[left] , lat[left].O[mu] ) ; 
    left = next ; 
    next = lat[ left ].neighbor[mu] ; 
  }
  return ;
}

//perform the maximal axial gauge fixing
static void 
MAG_fix( struct site *__restrict lat , 
	 GLU_complex *__restrict *__restrict gauge )
{
  // just in case set gauge to identity //
  int i ;
  #pragma omp parallel for private(i)
  PFOR( i = 0 ; i < LVOLUME ; i++ ) {
    identity( gauge[i] ) ; 
  }
  int subvol = 1 , mu ;
  for( mu = 0 ; mu < ND ; mu ++ ) {
    #pragma omp parallel for private(i)
    PFOR( i =  0 ; i < subvol ; i++ ) {
      gauge_set( lat , mu , i , gauge ) ;
    } 
    gtransform( lat , ( const GLU_complex ** )gauge ) ;
    subvol *= Latt.dims[ mu ] ;
  }
  return ;
}

// simplistic axial gauge calculation, A_{\mu} = 0 as best we can
void
axial_gauge( struct site *__restrict lat , 
	     GLU_complex *__restrict *__restrict gauge , 
	     const int DIR )
{
  int i ;
#pragma omp parallel for private(i)
  PFOR( i =  0 ; i < LCU ; i++ ) {// loop the ND - 1, subvolume
    // x is the ND dimensional vector describing the position
    int x[ ND ] ;
    get_mom_2piBZ( x , i , DIR ) ;
    const int k = gen_site( x ) ;
    gauge_set( lat , DIR , k , gauge ) ; 
  }
  gtransform( lat , ( const GLU_complex ** )gauge ) ;
  return ;
}

// wrapper for the mag-fixing bit ...
void 
mag( struct site *__restrict lat , 
     GLU_complex *__restrict *__restrict gauge )
{
  //loop lattice reuinitarizing everything
  latt_reunitU( lat ) ; 
  MAG_fix( lat , gauge ) ; 
  latt_reunitU( lat ) ; 
  return ;
}

// residual fix
void
residual_fix( struct site *__restrict lat )
{
  int i ;
  // oh god this is pretty weird, oh wait no I get it, 
  // it is like the axial gauge all over again ... 

  // temporal gauge transformation matrices
  GLU_complex **gauge = malloc( Latt.dims[ND-1] * sizeof( GLU_complex* ) ) ;
  for( i = 0 ; i < Latt.dims[ND-1] ; i++ ) {
    gauge[i] = ( GLU_complex* ) malloc( NCNC*sizeof( GLU_complex ) ) ;
  } 

  // puts the transform at slice "t" in gauge
  identity( gauge[0] ) ;
  const GLU_real one_LCU = 1.0 / LCU ; 
  int t ;
  for( t = 0 ; t < Latt.dims[ND-1]-1 ; t++ ) {
    GLU_complex sum[ NCNC ] = {} ;
    for( i = 0 ; i < LCU ; i++ ) {
      a_plus_b( sum , lat[ i + LCU*t ].O[ND-1] ) ;
    }
    // turn it into the average ... not needed?
    for( i = 0 ; i < NCNC ; i++ ) { sum[i] *= one_LCU ; }

    // project the sum of the temporal links back to SU(NC) two ways of doing this I can think of
    #ifdef HERM_PROJ
    // 1. Hermitian projection and exact exponentiation
    GLU_complex A[ NCNC ] ;
    Hermitian_proj( A , sum ) ;
    exponentiate( sum , A ) ;
    #else
    // 2. Simple reunitarisation
    givens_reunit( sum ) ;
    #endif

    // and multiply the next gauge transformations with the previous and this one!
    multab_suNC( gauge[t+1] , gauge[t] , sum ) ;
  } 
  // test whether this does actually diagonalise the sum
 #ifdef DEBUG_RESFIX
  for( t = 0 ; t < Latt.dims[ND-1] ; t++ ) {
    GLU_complex sum[ NCNC ] = {} ;
    for( i = 0 ; i < LCU ; i++ ) {
      a_plus_b( sum , lat[ i + LCU*t ].O[ND-1] ) ;
    }

    #ifdef HERM_PROJ
    GLU_complex A[ NCNC ] ;
    Hermitian_proj( A , sum ) ;
    exponentiate( sum , A ) ;
    #else
    givens_reunit( sum ) ;
    #endif

    GLU_complex temp[ NCNC ] ;
    const int tup = ( t == Latt.dims[ND-1]-1 ) ? 0 : t+1 ;
    multab_dag_suNC( temp , sum , gauge[tup] ) ;
    multab_suNC( sum , gauge[t] , temp ) ;
    write_matrix( sum ) ;
  }
#endif
#pragma omp parallel for private(i)
  for( i = 0 ; i < LVOLUME ; i++ ) {
    GLU_complex temp[ NCNC ] ;
    const int t = (int)( i / LCU ) ;
    const int tup = ( t == Latt.dims[ND-1]-1 ) ? 0 : t+1 ;
    multab_dag_suNC( temp , lat[i].O[ND-1] , gauge[tup] ) ;
    multab_suNC( lat[i].O[ND-1] , gauge[t] , temp ) ; 
    #ifdef SINGLE_PREC
    reunit2( lat[i].O[ND-1] ) ;
    #endif
    int mu ;
    for( mu = 0 ; mu < ND-1 ; mu ++ ) {
      multab_dag_suNC( temp , lat[i].O[mu] , gauge[t] ) ;
      multab_suNC( lat[i].O[mu] , gauge[t] , temp ) ; 
      #ifdef SINGLE_PREC
      reunit2( lat[i].O[mu] ) ;
      #endif
    }
  }
  return ;
}