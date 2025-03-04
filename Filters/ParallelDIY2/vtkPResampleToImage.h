/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPResampleToImage.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPResampleToImage - sample dataset on a uniform grid in parallel
// .SECTION Description
// vtkPResampleToImage is a parallel filter that resamples the input dataset on
// a uniform grid. It internally uses vtkProbeFilter to do the probing.
// .SECTION See Also
// vtkResampleToImage vtkProbeFilter

#ifndef vtkPResampleToImage_h
#define vtkPResampleToImage_h

#include "vtkFiltersParallelDIY2Module.h" // For export macro
#include "vtkResampleToImage.h"

class vtkDataSet;
class vtkImageData;

class VTKFILTERSPARALLELDIY2_EXPORT vtkPResampleToImage : public vtkResampleToImage
{
public:
  vtkTypeMacro(vtkPResampleToImage, vtkResampleToImage);
  void PrintSelf(ostream& os, vtkIndent indent);

  static vtkPResampleToImage *New();

protected:
  vtkPResampleToImage();
  ~vtkPResampleToImage();

  virtual int RequestData(vtkInformation *, vtkInformationVector **,
                          vtkInformationVector *);

private:
  vtkPResampleToImage(const vtkPResampleToImage&);  // Not implemented.
  void operator=(const vtkPResampleToImage&);  // Not implemented.
};

#endif
