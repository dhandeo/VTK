/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPTIFWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPTIFWriter.h"

#include "vtkErrorCode.h"
#include "vtkImageData.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkJPEGWriter.h"
#include "vtkNew.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include "vtkStreamingDemandDrivenPipeline.h"


#if _MSC_VER
#define snprintf _snprintf
#endif

vtkStandardNewMacro(vtkPTIFWriter);

//----------------------------------------------------------------------------
vtkPTIFWriter::vtkPTIFWriter()
  : TIFFPtr(NULL), Width(0), Height(0), Pages(0),
    XResolution(-1.0), YResolution(-1.0), JPEGQuality(75), TileSize(256)
{
  this->SetPadding(255, 255, 255);
}

//----------------------------------------------------------------------------
void vtkPTIFWriter::Write()
{
  // make sure the latest input is available.
  this->GetInputAlgorithm()->UpdateInformation();
  this->SetErrorCode(vtkErrorCode::NoError);
  // Error checking
  if (this->GetInput() == NULL)
    {
    vtkErrorMacro(<<"Write: Please specify an input!");
    return;
    }
  if (!this->FileName)
    {
    vtkErrorMacro(<<"Write: Please specify an output FileName");
    this->SetErrorCode(vtkErrorCode::NoFileNameError);
    return;
    }

  // Fill in image information.
  this->GetInputExecutive(0, 0)->UpdateInformation();
  int *wExtent;
  wExtent = vtkStreamingDemandDrivenPipeline::GetWholeExtent(
    this->GetInputInformation(0, 0));

  this->UpdateProgress(0.0);

  // this->WriteFileHeader(0, this->GetInput(), wExtent);

  // Now stream the data
  int extent[6];

  extent[0] = 0;
  extent[1] = this->TileSize -1;
  extent[2] = 0;
  extent[3] = this->TileSize -1;
  extent[4] = 0;
  extent[5] = 0;
  for(int i=0; i < 6; i ++) this->DataUpdateExtent[i] = extent[i];

  vtkStreamingDemandDrivenPipeline* exec = vtkStreamingDemandDrivenPipeline::SafeDownCast(this->GetExecutive());
  exec->SetUpdateExtent(this->GetInputInformation(0, 0), extent);

  // vtkStreamingDemandDrivenPipeline::SetUpdateExtent(
  //   this->GetInputInformation(0, 0), extent);
  //
  int *uExtent;
  uExtent = vtkStreamingDemandDrivenPipeline::GetUpdateExtent(
    this->GetInputInformation(0, 0));
  cout << "UpdateExtents" << uExtent[0] << ", " << uExtent[1] << endl;
  this->Update();

  // vtkImageData *input = this->GetInput();
  // int dim[3];
  // cout << "Dims: " << dim[0] << ", " << dim[1] << endl;
  this->WriteTile(0,this->GetInput() , extent, 0);
  // this->WriteFileTrailer(0, 0);
}

//---------------------------------------------------------------------------
int vtkPTIFWriter::RequestInformation(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *vtkNotUsed(outputVector))
{
  // Check to make sure that all input information agrees
  // this->MismatchedInputs = 0;

  double spacing[3];
  double origin[3];
  int extent[6];
  int components = 0;
  int dataType = 0;

  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  inInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), extent);
  inInfo->Get(vtkDataObject::SPACING(), spacing);
  inInfo->Get(vtkDataObject::ORIGIN(), origin);
  components = inInfo->Get(vtkDataObject::FIELD_NUMBER_OF_COMPONENTS());
  dataType = inInfo->Get(vtkDataObject::FIELD_ARRAY_TYPE());

  for(int i=0; i < 6; i ++) this->DataUpdateExtent[i] = extent[i];
  cout << "RequestInformation: " << extent[0] << ", " << extent[1] << endl;
  return 1;
}


//--------------------------------------------------------------------------
int vtkPTIFWriter::RequestUpdateExtent(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *vtkNotUsed(outputVector))
{
  // Set the UpdateExtent from the DataUpdateExtent for the current slice
  int n = inputVector[0]->GetNumberOfInformationObjects();
  cout << "RequestUpdateExtent: " << n << "Information  Objects" << endl;
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);

  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
              this->DataUpdateExtent, 6);
  return 1;
}

//--------------------------------------------------------------------------
int vtkPTIFWriter::RequestData(
  vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector,
  vtkInformationVector* vtkNotUsed(outputVector))
{
    vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
    vtkImageData *input = vtkImageData::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));

    // Check the extents being requested
    int dim[3];
    input->GetDimensions(dim);

    int *extents;
    extents = vtkStreamingDemandDrivenPipeline::GetUpdateExtent(inInfo);
    cout << "RequestData: " << extents[0] << ", " << extents[1] << endl;
    cout << "InputDims: " << dim[0] << ", " << dim[1] << endl;
    // // Error checking
    // if (input == NULL)
    //   {
    //   // Close file, set MINCFileID to zero
    //   this->CloseNetCDFFile(this->MINCFileId);
    //   this->MINCFileId = 0;
    //   vtkErrorMacro(<<"Write: Please specify an input!");
    //   return 0;
    //   }
    //
    // // Call WriteMINCData for each input
    // if (this->WriteMINCData(
    //       input,
    //       timeStep,
    //       vtkStreamingDemandDrivenPipeline::GetWholeExtent(inInfo),
    //       vtkStreamingDemandDrivenPipeline::GetUpdateExtent(inInfo)) == 0)
    //   {
    //   return 0;
    //   }
    // }

  return 1;
}

//----------------------------------------------------------------------------
void vtkPTIFWriter::WriteFileHeader(ofstream *, vtkImageData *data, int wExt[6])
{
  int dims[3];
  data->GetDimensions(dims);
  int scomponents = data->GetNumberOfScalarComponents();
  int stype = data->GetScalarType();

  // Make sure we have data in correct format
  if(stype != VTK_UNSIGNED_CHAR)
    {
    vtkErrorMacro(<< "Unsupported data type: " << data->GetScalarTypeAsString());
    this->SetErrorCode(vtkErrorCode::FileFormatError);
    return;
    }
  if(scomponents != 3)
    {
    vtkErrorMacro(<< "Only supporting rgb color, not number of components: " << scomponents);
    this->SetErrorCode(vtkErrorCode::FileFormatError);
    return;
    }

  // Find the width/height of the images
  this->Width = wExt[1] - wExt[0] + 1;
  this->Height = wExt[3] - wExt[2] + 1;
  // Check if we need to write an image stack (pages > 2).
  this->Pages = wExt[5] - wExt[4] + 1;

  cout << this->FileName << endl;
  cout << "Width: " << this->Width << endl;
  cout << "Height: " << this->Height << endl;
  cout << "Pages: " << this->Pages << endl;

  // Check the resolution too, assume we store it in metric (as in reader).
  this->XResolution = 10.0 / data->GetSpacing()[0];
  this->YResolution = 10.0 / data->GetSpacing()[1];

  TIFF* tif = TIFFOpen(this->FileName, "w");
  if(tif == NULL)
    {
    vtkErrorMacro(<< "Could not create output file " << this->FileName);
    this->SetErrorCode(vtkErrorCode::FileFormatError);
    return;
    }
  this->TIFFPtr = tif;

  cout << "Done opening .." << endl;

  uint32 w = this->Width;
  uint32 h = this->Height;

  // Set mostly default tif tags
  TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, 100);
  TIFFSetField(tif, TIFFTAG_IMAGELENGTH, 100);
  TIFFSetField(tif, TIFFTAG_TILEWIDTH, this->TileSize);
  TIFFSetField(tif, TIFFTAG_TILELENGTH, this->TileSize);
  TIFFSetField(tif, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
  TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, 3); // Ignore alpha
  TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, 8); // Always same from openslide reader
  TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
  TIFFSetField(tif, TIFFTAG_COMPRESSION, 7); // COMPRESSION_JPEG
  TIFFSetField(tif, TIFFTAG_JPEGQUALITY, this->JPEGQuality);
  TIFFSetField(tif, TIFFTAG_JPEGCOLORMODE, JPEGCOLORMODE_RGB);
  TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, 6); // Always same for JPEG

  // Make sure that the
  if (tif == NULL)
    {
    vtkErrorMacro("Problem writing header.");
    this->SetErrorCode(vtkErrorCode::FileFormatError);
    return;
    }
  }

//----------------------------------------------------------------------------
void vtkPTIFWriter::WriteFile(ofstream *, vtkImageData *data,
                              int extent[6], int*)
{
  // // Make sure we actually have data.
  // if (!data->GetPointData()->GetScalars())
  //   {
  //   vtkErrorMacro(<< "Could not get data from input.");
  //   return;
  //   }
  if (this->TIFFPtr == NULL)
    {
    vtkErrorMacro("Problem writing data.");
    this->SetErrorCode(vtkErrorCode::FileFormatError);
    return;
    }
  //
  // // take into consideration the scalar type
  // if( data->GetScalarType() != VTK_UNSIGNED_CHAR
  //  && data->GetScalarType() != VTK_UNSIGNED_SHORT
  //  && data->GetScalarType() != VTK_FLOAT)
  //   {
  //   vtkErrorMacro("TIFFWriter only accepts unsigned char/short or float scalars!");
  //   return;
  //   }

  // if (this->Pages > 1)
  //   {
  //   // Call the correct templated function for the input
  //   void *inPtr = data->GetScalarPointer();
  //
  //   switch (data->GetScalarType())
  //     {
  //     vtkTemplateMacro(this->WriteVolume((VTK_TT *)(inPtr)));
  //     default:
  //       vtkErrorMacro("UpdateFromFile: Unknown data type");
  //     }
  //   }
  // else
  //   {
  //   // Now write the image for the current pfpt[5]; ++idx2)
  //     {
  //     for (int idx1 = extent[3]; idx1 >= extent[2]; idx1--)
  //       {
  //       void *ptr = data->GetScalarPointer(extent[0], idx1, idx2);
  //       if (TIFFWriteScanline(tif, static_cast<unsigned char*>(ptr), row, 0) < 0)
  //         {
  //         this->SetErrtiforCode(vtkErrorCode::OutOfDiskSpaceError);
  //         break;
  //         }
  //       ++row;
  //       }
  //     }
  //   }
}


void vtkPTIFWriter::WriteTile(ofstream *, vtkImageData *data,
                              int extent[6], int*)
{
  // Compute tile name
  // Make sure we actually have data.
  int ex[6];
  data->GetExtent(ex);
  cout << "Data: " << ex[0] << ", " << ex[1] << endl;
  cout << "Extent: " << extent[0] << ", " << extent[1] << endl;


  vtkNew<vtkJPEGWriter> vtkWr;
  vtkWr->SetFileName("temp.jpg");
  vtkWr->SetQuality(70);
  vtkWr->ProgressiveOff();
  vtkWr->SetInputData(data);
  vtkWr->Write();
  return;

  if (!data->GetPointData()->GetScalars())
    {
    vtkErrorMacro(<< "Could not get data from input.");
    return;
    }

  if (this->TIFFPtr == NULL)
    {
    vtkErrorMacro("Problem writing data.");
    this->SetErrorCode(vtkErrorCode::FileFormatError);
    return;
    }

  void *inPtr = data->GetScalarPointer();
  if (TIFFWriteRawTile(this->TIFFPtr, 0, static_cast<unsigned char*>(inPtr), 100*100) < 0)
    {
    vtkErrorMacro(<< "Error writing tile");
    this->SetErrorCode(vtkErrorCode::FileFormatError);
    }
}


//----------------------------------------------------------------------------
void vtkPTIFWriter::WriteFileTrailer(ofstream *, vtkImageData *)
{
  if(this->TIFFPtr)
    {
    TIFFClose(this->TIFFPtr);
    }
  else
    {
    vtkErrorMacro("Problem writing trailer.");
    this->SetErrorCode(vtkErrorCode::FileFormatError);
    }

  this->TIFFPtr = 0;
}

//----------------------------------------------------------------------------
void vtkPTIFWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Compression: ";
  // if ( this->Compression == vtkPTIFWriter::PackBits )
  //   {
  //   os << "Pack Bits\n";
  //   }
  // else if ( this->Compression == vtkPTIFWriter::JPEG )
  //   {
  //   os << "JPEG\n";
  //   }
  // else if ( this->Compression == vtkPTIFWriter::Deflate )
  //   {
  //   os << "Deflate\n";
  //   }
  // else if ( this->Compression == vtkPTIFWriter::LZW )
  //   {
  //   os << "LZW\n";
  //   }
  // else //if ( this->Compression == vtkPTIFWriter::NoCompression )
  //   {
  //   os << "No Compression\n";
  //   }
}
