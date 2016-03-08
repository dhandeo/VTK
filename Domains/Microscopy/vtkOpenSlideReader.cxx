/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenSlideReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOpenSlideReader.h"

#include "vtkDataArray.h"
#include "vtkImageData.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkToolkits.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro(vtkOpenSlideReader);

void vtkOpenSlideReader::ExecuteInformation()
{
  //std::cout << this->GetFileName() << std::endl;

  this->openslide_handle = openslide_open(this->GetFileName());

  if(this->openslide_handle == NULL || openslide_get_error(this->openslide_handle) != NULL)
    {
    vtkErrorWithObjectMacro(this,
                            "File could not be opened by openslide"
                           );
    return;
    }

  int64_t w, h;
  openslide_get_level0_dimensions(this->openslide_handle, &w, &h);
  // cout << "OpenSlideInfDims: " << w << ", " << h << endl;

  this->vtkImageReader2::ExecuteInformation();

  this->DataExtent[0] = 0;
  this->DataExtent[1] = w-1;
  this->DataExtent[2] = 0;
  this->DataExtent[3] = h-1;
  this->DataExtent[4] = 0;
  this->DataExtent[5] = 0;

  this->SetNumberOfScalarComponents(3);
  this->SetDataScalarTypeToUnsignedChar();
}


//----------------------------------------------------------------------------
// This function reads a data from a file.  The datas extent/axes
// are assumed to be the same as the file extent/order.
void vtkOpenSlideReader::ExecuteDataWithInformation(vtkDataObject *output,
                                               vtkInformation *outInfo)
{
  int inExtent[6];

  vtkStreamingDemandDrivenPipeline::GetUpdateExtent(
      outInfo,
      inExtent);

  // cout << "OpenSlideReaderDataInf: " << inExtent[0] << ", " << inExtent[1] << ", " << inExtent[2] << ", " << inExtent[3] << endl;

  vtkImageData *data = this->AllocateOutputData(output, outInfo);
  //data->GetExtent(this->OutputExtent);
  //data->GetIncrements(this->OutputIncrements);

  if(this->openslide_handle == NULL)
    {
    std::cout << "In the data info update, file is not updated" << std::endl;
    }

  //std::cout << "Extents: " << data->GetExtent() << std::endl;

  this->ComputeDataIncrements();

  data->GetPointData()->GetScalars()->SetName("OpenSlideImage");
  //// Leverage openslide to read the region

  // int inExtent[6];
  // data->GetExtent(inExtent);
  // cout << inExtent[0] << ", " << inExtent[1] << endl;
  // cout << inExtent[2] << ", " << inExtent[3] << endl;

  int w = inExtent[1] - inExtent[0] + 1;
  int h = inExtent[3]- inExtent[2] + 1;
  unsigned char * buffer = new unsigned char[w * h * 4];

  openslide_read_region(this->openslide_handle, (unsigned int *) buffer,
    inExtent[0],
    inExtent[2],
    0,  // level
    w,
    h
    );

  if(openslide_get_error(this->openslide_handle) != NULL)
    {
    delete[] buffer;
    vtkErrorWithObjectMacro(this,
                            "File could not be read by openslide"
                           );
    return;
    }

  unsigned char* outputPtr = (unsigned char*)(data->GetScalarPointer());
  unsigned char* bufPtr = (unsigned char*) buffer;

  // Order = RGBA
  for (long y=0; y < h; y++)
    {
    for(long x=0; x < w; x++)
      {
      unsigned char* rgba = &bufPtr[((h-1-y)*w + x) * 4];
      unsigned char* rgb =  &outputPtr[(y*w + x) * 3];
      // Convert from BGRA to RGB
      rgb[2] = rgba[0];
      rgb[1] = rgba[1];
      rgb[0] = rgba[2];
      }
    }

  delete[] buffer;
  openslide_close(this->openslide_handle);
}


//----------------------------------------------------------------------------
int vtkOpenSlideReader::CanReadFile(const char* fname)
{
  return 3;
}


//----------------------------------------------------------------------------
void vtkOpenSlideReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
