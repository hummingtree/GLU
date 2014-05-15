/*
    Copyright 2013 Renwick James Hudspith

    This file (taylor_logs.h) is part of GLU.

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
   @file taylor_logs.h
   @brief Taylor series approximations to the logarithm of a link matrix.
 */

#ifndef GLU_TAYLOR_LOGS_H
#define GLU_TAYLOR_LOGS_H

/**
   @fn void brute_force_log( GLU_complex *__restrict Q , const GLU_complex *__restrict U , const int NROOTS ) ;
   @brief computes the logarithm of a matrix using accelerated Taylor series approximations
   @param Q :: the principal logarithm of Q
   @param U :: the matrix having its log taken
   @param NROOTS :: the number of nested roots we use for convergence
   This code is as near as possible generic for any matrix arguments. As it is
   based on series expansions it is dreadfully slow, but it is useful.
 */
void
brute_force_log( GLU_complex *__restrict Q , 
		 const GLU_complex *__restrict U ,
		 const int NROOTS ) ;

/**
   @fn free_taylors( void )
   @brief frees the Taylor expansion coefficients for the log
 */
void
free_taylors( void ) ;

/**
   @fn void nape_reunit( GLU_complex *__restrict U )
   @brief reunitarizes a matrix to SU(N) using the n-APE projection
   @param U :: matrix being reunitarised
 */
void
nape_reunit( GLU_complex *__restrict U ) ;


#endif