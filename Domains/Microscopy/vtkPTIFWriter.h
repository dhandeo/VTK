/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPTIFWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPTIFWriter - read digital whole slide images supported by
// PTIF library
// .SECTION Description
// vtkPTIFWriter is a source object that uses openslide library to
// read multiple supported image formats used for whole slide images in
// microscopy community.
//
// .SECTION See Also
// vtkPTIFWriter

#ifndef vtkPTIFWriter_h
#define vtkPTIFWriter_h

#include "vtkIOImageModule.h" // For export macro
#include "vtkThreadedImageAlgorithm.h"



class VTKIOIMAGE_EXPORT vtkPTIFWriter : public vtkThreadedImageAlgorithm
{
public:
  static vtkPTIFWriter *New();
  vtkTypeMacro(vtkPTIFWriter,vtkThreadedImageAlgorithm);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkPTIFWriter();
  ~vtkPTIFWriter() {}
  virtual void  ThreadedExecute (vtkImageData *inData, vtkImageData *outData, int extent[6], int threadId);
  virtual void  ThreadedRequestData (vtkInformation *request, vtkInformationVector **inputVector,
     vtkInformationVector *outputVector, vtkImageData ***inData,
     vtkImageData **outData, int extent[6], int threadId);

  virtual int RequestInformation (vtkInformation *, vtkInformationVector**, vtkInformationVector *);
private:

  vtkPTIFWriter(const vtkPTIFWriter&);  // Not implemented.
  void operator=(const vtkPTIFWriter&);  // Not implemented.
};
#endif
