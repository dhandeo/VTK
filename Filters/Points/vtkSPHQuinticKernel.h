/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSPHQuinticKernel.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSPHQuinticKernel - a quintic SPH interpolation kernel

// .SECTION Description
// vtkSPHQuinticKernel is an smooth particle hydrodynamics interpolation kernel as
// described by D.J. Price. This is a quintic formulation.
//
// .SECTION Caveats
// See D.J. Price, Smoothed particle hydrodynamics and magnetohydrodynamics,
// J. Comput. Phys. 231:759-794, 2012. Especially equation 49.

// .SECTION See Also
// vtkSPHKernel vtkSPHInterpolator


#ifndef vtkSPHQuinticKernel_h
#define vtkSPHQuinticKernel_h

#include "vtkFiltersPointsModule.h" // For export macro
#include "vtkSPHKernel.h"
#include <algorithm> // For std::min()

class vtkIdList;
class vtkDoubleArray;


class VTKFILTERSPOINTS_EXPORT vtkSPHQuinticKernel : public vtkSPHKernel
{
public:
  // Description:
  // Standard methods for instantiation, obtaining type information, and printing.
  static vtkSPHQuinticKernel *New();
  vtkTypeMacro(vtkSPHQuinticKernel,vtkSPHKernel);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Produce the computational parameters for this kernel. Invoke this method
  // after setting initial values like SpatialStep.
  virtual void Initialize(vtkAbstractPointLocator *loc, vtkDataSet *ds,
                          vtkPointData *pd);

  // Description:
  // Given a point x, and a list of basis points pIds, compute interpolation
  // weights associated with these basis points.
  virtual vtkIdType ComputeWeights(double x[3], vtkIdList *pIds,
                                   vtkDoubleArray *weights);

  // Description:
  // Given a point x, and a list of basis points pIds, compute interpolation
  // weights, plus derivative weights, associated with these basis points.
  virtual vtkIdType ComputeGradWeights(double x[3], vtkIdList *pIds,
                                       vtkDoubleArray *weights,
                                       vtkDoubleArray *gradWeights);

  // Description:
  // Compute weighting factor given a normalized distance from a sample point.
  double ComputeFunctionWeight(const double d)
  {
    double tmp1 = 3.0 - std::min(d,3.0);
    double tmp2 = 2.0 - std::min(d,2.0);
    double tmp3 = 1.0 - std::min(d,1.0);
    return (tmp1*tmp1*tmp1*tmp1*tmp1 - 6.0*tmp2*tmp2*tmp2*tmp2*tmp2 +
            15.0*tmp3*tmp3*tmp3*tmp3*tmp3);
  }

  // Description:
  // Compute weighting factor for derivative quantities given a normalized
  // distance from a sample point.
  double ComputeGradientWeight(const double d)
  {
    double tmp1 = 3.0 - std::min(d,3.0);
    double tmp2 = 2.0 - std::min(d,2.0);
    double tmp3 = 1.0 - std::min(d,1.0);
    return (tmp1*tmp1*tmp1*tmp1 - 6.0*tmp2*tmp2*tmp2*tmp2 +
            15.0*tmp3*tmp3*tmp3*tmp3);
  }

protected:
  vtkSPHQuinticKernel();
  ~vtkSPHQuinticKernel();

private:
  vtkSPHQuinticKernel(const vtkSPHQuinticKernel&);  // Not implemented.
  void operator=(const vtkSPHQuinticKernel&);  // Not implemented.
};

#endif
