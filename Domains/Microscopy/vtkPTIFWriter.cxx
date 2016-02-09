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

#include "vtkPTIFWriter.h"
#include "vtkImageData.h"
#include "vtkThreadedImageAlgorithm.h"
#include "vtkImageData.h"
#include "vtkImageProgressIterator.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"


vtkStandardNewMacro(vtkPTIFWriter);

//----------------------------------------------------------------------------
vtkPTIFWriter::vtkPTIFWriter()
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

//----------------------------------------------------------------------------
// Just change the Image type.
int vtkPTIFWriter::RequestInformation (
  vtkInformation       * vtkNotUsed( request ),
  vtkInformationVector** vtkNotUsed( inputVector ),
  vtkInformationVector * outputVector)
{
  // get the info objects
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  //vtkDataObject::SetPointDataActiveScalarInfo(outInfo, this->OutputScalarType, -1);
  return 1;
}


//----------------------------------------------------------------------------
// This templated function executes the filter for any type of data.
template <class IT, class OT>
void vtkPTIFWriterExecute(vtkPTIFWriter *self,
                         vtkImageData *inData,
                         vtkImageData *outData,
                         int outExt[6], int id, IT *, OT *)
{
}



//----------------------------------------------------------------------------
template <class T>
void vtkPTIFWriterExecute(vtkPTIFWriter *self,
                         vtkImageData *inData,
                         vtkImageData *outData, int outExt[6], int id,
                         T *)
{
  switch (outData->GetScalarType())
    {
    vtkTemplateMacro(vtkPTIFWriterExecute(self,
                                         inData, outData, outExt, id,
                                         static_cast<T *>(0),
                                         static_cast<VTK_TT *>(0)));
    default:
      vtkGenericWarningMacro("Execute: Unknown output ScalarType");
      return;
    }
}

//----------------------------------------------------------------------------
void vtkPTIFWriter::ThreadedRequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *vtkNotUsed(outputVector),
  vtkImageData ***inData,
  vtkImageData **outData,
  int outExt[6], int id)
{
}


//----------------------------------------------------------------------------
// This method is passed a input and output region, and executes the filter
// algorithm to fill the output from the input.
// It just executes a switch statement to call the correct function for
// the regions data types.
void vtkPTIFWriter::ThreadedExecute (vtkImageData *inData,
                                   vtkImageData *outData,
                                   int outExt[6], int id)
{
  switch (inData->GetScalarType())
    {
    vtkTemplateMacro(
      vtkPTIFWriterExecute(this, inData,
                          outData, outExt, id,
                          static_cast<VTK_TT *>(0)));
    default:
      vtkErrorMacro(<< "Execute: Unknown input ScalarType");
      return;
    }
}

//----------------------------------------------------------------------------
void vtkPTIFWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  //os << indent << "OutputScalarType: " << this->OutputScalarType << "\n";
  //os << indent << "ClampOverflow: ";
  //if (this->ClampOverflow)
    //{
    //os << "On\n";
    //}
  //else
    //{
    //os << "Off\n";
    //}
}
