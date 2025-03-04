/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeneralizedKernel.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkGeneralizedKernel.h"
#include "vtkAbstractPointLocator.h"
#include "vtkObjectFactory.h"
#include "vtkIdList.h"
#include "vtkDoubleArray.h"
#include "vtkDataSet.h"
#include "vtkPointData.h"
#include "vtkMath.h"

//----------------------------------------------------------------------------
vtkGeneralizedKernel::vtkGeneralizedKernel()
{
  this->KernelFootprint = vtkGeneralizedKernel::RADIUS;
  this->Radius = 1.0;
  this->NumberOfPoints = 8;
}


//----------------------------------------------------------------------------
vtkGeneralizedKernel::~vtkGeneralizedKernel()
{
}

//----------------------------------------------------------------------------
vtkIdType vtkGeneralizedKernel::
ComputeBasis(double x[3], vtkIdList *pIds)
{
  if ( this->KernelFootprint == vtkGeneralizedKernel::RADIUS )
    {
    this->Locator->FindPointsWithinRadius(this->Radius, x, pIds);
    }
  else
    {
    this->Locator->FindClosestNPoints(this->NumberOfPoints, x, pIds);
    }

  return pIds->GetNumberOfIds();
}

//----------------------------------------------------------------------------
void vtkGeneralizedKernel::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Kernel Footprint: " << this->KernelFootprint << "\n";
  os << indent << "Radius: " << this->Radius << "\n";
  os << indent << "Number of Points: " << this->NumberOfPoints << "\n";

}
