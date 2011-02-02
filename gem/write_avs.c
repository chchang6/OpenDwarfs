/****************************************************************************
 * GEM -- electrostatics calculations and visualization                     *
 * Copyright (C) 2006  John C. Gordon                                       *
 *                                                                          *
 * This program is free software; you can redistribute it and/or modify     *
 * it under the terms of the GNU General Public License as published by     *
 * the Free Software Foundation; either version 2 of the License, or        *
 * (at your option) any later version.                                      *
 *                                                                          *
 * This program is distributed in the hope that it will be useful,          *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 * GNU General Public License for more details.                             *
 *                                                                          *
 * You should have received a copy of the GNU General Public License along  *
 * with this program; if not, write to the Free Software Foundation, Inc.,  *
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.              *
 ****************************************************************************/

/*
 * This header writing code was janked out of AvsScalarField in the MEAD distro.
 * We do this to ensure that GEM grids can be read in by MEAD -- since AVS is 
 * such a broad and complicated file format and MEAD uses just a small
 * portion of its capabilities for data representation and storage.  (No sense
 * in handling the whole AVS format for only a small subset of its capabilities
 * anyway) --jcg
 */

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "defines.h"
#include "file_io.h"

/***************************************************************************
 * FUNCTION: write_AVS_header   --writes an AVS format header to a file    *
 *                                                                         *
 * INPUTS: fp   --file pointer to read header from                         *
 *         dim  -- dimension of the grid <int>                             *
 *         xmin, ymin, zmin  -- lower back left corner of cube             *
 *         xmax, ymax, zmax  -- top front right corner of cube             *
 *         gridtype          -- type of grid (should be 'f')               *
 *                                                                         *
 * RETURNS: 1 on success, 0 on failure                                     *
 *                                                                         *
 ***************************************************************************/
static int write_AVS_header (FILE *fp, int dim, float xmin, float ymin, float zmin, float xmax, float ymax, float zmax, char gridtype)
{
   /* local constants */
   /*******************/
   const char FORM_FEEDS[2] = {12, 12}; /* header ends with 2 of these */

   float center_x, center_y, center_z, spacing;
   time_t tclock = time(0);


   if (fp == NULL) return 0;

   /* center */
   printf("writing an AVS header, min (%f, %f, %f): max (%f, %f, %f)\n",
              xmin, ymin, zmin, xmax, ymax, zmax);

   center_x = (xmin + xmax) / 2.;
   center_y = (ymin + ymax) / 2.;
   center_z = (zmin + zmax) / 2.;
   spacing =  (xmax - xmin) / (float)(dim-1);

   /* actual header data */
   fprintf(fp, "# AVS field file -- generated by GEM\n");
   fprintf(fp, "# creation date: %s", ctime(&tclock));
   fprintf(fp, "# Scalar field for cubic lattice: ON_GEOM_CENT resolved to (%f %f %f) %i %f \n",
               center_x, center_y, center_z, dim, spacing);
   fprintf(fp, "ndim=3         # number of dimensions in the field \n");
   fprintf(fp, "dim1=%i # dimension of axis 1\n", dim);
   fprintf(fp, "dim2=%i # dimension of axis 2\n", dim);
   fprintf(fp, "dim3=%i # dimension of axis 3\n", dim);
   fprintf(fp, "nspace=3       # number of physical coordinates per point \n");
   fprintf(fp, "veclen=1       # number of data components per point \n");
   fprintf(fp, "data=%s    # data type (byte, integer, float, double) \n",
               ((gridtype == 'f')?"float":"double"));
   fprintf(fp, "min_ext=%f %f %f\n", xmin, ymin, zmin);
   fprintf(fp, "max_ext=%f %f %f\n", xmax, ymax, zmax);
   fprintf(fp, "field=uniform  # field type (uniform, rectilinear, irregular)\n");

   /* double form feed at the end of the header */
   fwrite(FORM_FEEDS, sizeof(char), 2, fp); 

   return 1;
}

/***************************************************************************
 * FUNCTION: write_AVS_binary   --writes binary portion of AVS file        *
 *                                                                         *
 * INPUTS: fp   --file pointer to read header from                         *
 *         type --type of element                                          *
 *         dim  --size of the grid <int>                       *
 *         grd  -- grid data                                   *
 *                                                                         *
 * RETURNS: 1 on success, 0 on failure                                     *
 *                                                                         *
 ***************************************************************************/
static int write_AVS_binary (FILE *fp, void *grd, int dim, char type)
{
/* local variables */
/*******************/
int  written,
     nmemb = dim*dim*dim,
     i,
     data_size;

double *dgrid = NULL;
float  *fgrid = NULL;

   if (type == 'd')
   {
      dgrid = (double *)grd;

      for (i = 0; i < nmemb; i++)
          dgrid[i] /= ELECTROSTATIC_CONVERSION_FACTOR;

      data_size = sizeof(double);
   }
   else
   {
      fgrid = (float *)grd;

      for (i = 0; i < nmemb; i++)
          fgrid[i] /= ELECTROSTATIC_CONVERSION_FACTOR;

      data_size = sizeof(float);
   }

   written = fwrite(grd, data_size, nmemb, fp);

   return ( (written == nmemb)? 1 : 0 );
}


/***************************************************************************
 * FUNCTION: write_avs   --writes out an avs file                          *
 *                                                                         *
 * INPUTS: fname --file name to open                                       *
 *         grd               -- grid data                                  *
 *         dim               -- size of the grid <int>                     *
 *         xmin, ymin, zmin  -- bottom, back, left corner of the grid      *
 *         xmax, ymax, zmax  -- front, top, right corner of the grid       *
 *                                                                         *
 * RETURNS: 1 on success, 0 on failure                                     *
 *                                                                         *
 ***************************************************************************/
int write_avs (char *fname, float *grd, int dim, float xmin, float ymin, float zmin, float xmax, float ymax, float zmax)
{
   /* local variables */
   /*******************/

   FILE *fp; /* input file stream for the grid file  */

   if ((fname == NULL) || (grd == NULL)) return 0;

   /* simple sanity */
   if ((fp = fopen(fname, "wb")) == NULL)
   {
      fprintf(stderr, "Could not open file to write\n");
      return 0;
   }

   if (!write_AVS_header(fp, dim, xmin, ymin, zmin, xmax, ymax, zmax, 'f'))
   {
      fprintf(stderr, "Could not write AVS header information\n");
      return 0;
   }

   if (!write_AVS_binary (fp, grd, dim, 'f'))
   {
      fprintf (stderr, "Could not write AVS binary information\n");
      return 0;
   }

   /* toss on the tail */
   fwrite(&xmin, sizeof(float), 1, fp); 
   fwrite(&xmax, sizeof(float), 1, fp); 
   fwrite(&ymin, sizeof(float), 1, fp); 
   fwrite(&ymax, sizeof(float), 1, fp); 
   fwrite(&zmin, sizeof(float), 1, fp); 
   fwrite(&zmax, sizeof(float), 1, fp); 

   fclose(fp);
     
   /* seems like everything went ok */
   return 1;
}
