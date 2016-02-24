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
#include <stack>
#include <string>


#if _MSC_VER
#define snprintf _snprintf
#endif

vtkStandardNewMacro(vtkPTIFWriter);

//----------------------------------------------------------------------------
vtkPTIFWriter::vtkPTIFWriter()
  : TIFFPtr(NULL), Width(0), Height(0), Pages(0), CurDir(0),
    XResolution(-1.0), YResolution(-1.0), JPEGQuality(75), TileSize(256)
{
  this->SetPadding(255, 255, 255);
}

//----------------------------------------------------------------------------
void vtkPTIFWriter::Write()
{
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
  vtkDemandDrivenPipeline::SafeDownCast(
    this->GetInputExecutive(0, 0))->UpdateInformation();
  this->UpdateInformation();

  // int *wExtent;
  // wExtent = vtkStreamingDemandDrivenPipeline::GetWholeExtent(
  //   this->GetInputInformation(0, 0));
  //
  // // vtkInformation *inf = this->(0, 0);
  // // inf->Print(cout);

  this->UpdateProgress(0.0);

  // Updating is the responsibility of subfunction
  this->WriteFileHeader(0, 0, 0);
  //
  // // Now stream the data
  int extent[6];

  extent[0] = 0;
  extent[1] = this->TileSize -1;
  extent[2] = 0;
  extent[3] = this->TileSize -1;
  extent[4] = 0;
  extent[5] = 0;
  for(int i=0; i < 6; i ++) this->DataUpdateExtent[i] = extent[i];

  // vtkImageData *input = this->GetInput();
  // cout << "Dims: " << dim[0] << ", " << dim[1] << endl;
  // int dim[3];
  this->WriteFile(0,0,extent,0);
  // this->WriteTile(0, this->GetInput(), extent, 0);
  this->WriteFileTrailer(0, 0);
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
  int hup = this->GetInput()->GetScalarType(inInfo);
  cout << "DataType: " << hup << " Should be " << VTK_UNSIGNED_CHAR << endl;

  this->DataType = dataType;
  this->NumScalars = components;

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

  return 1;
}

void vtkPTIFWriter::SelectDirectory(int dir)
{

  if(this->CurDir != dir) {
    TIFFSetDirectory(this->TIFFPtr, dir);
  }

}

//----------------------------------------------------------------------------
void vtkPTIFWriter::WriteFileHeader(ofstream *, vtkImageData *data2, int wExt[6])
{
  int dims[3];
  cout << "InHeader " << endl;
  this->UpdateInformation();

  // Get the input information
  int extent[6];
  vtkStreamingDemandDrivenPipeline::GetWholeExtent(
    this->GetInputInformation(0, 0), extent);

  vtkStreamingDemandDrivenPipeline* exec = vtkStreamingDemandDrivenPipeline::SafeDownCast(this->GetExecutive());

  vtkImageData * data = this->GetInput();
  int stype = data->GetScalarType(this->GetInputInformation(0, 0));
  int scomponents = data->GetNumberOfScalarComponents(this->GetInputInformation(0, 0));

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
  this->Width = extent[1] - extent[0] + 1;
  this->Height = extent[3] - extent[2] + 1;
  // Check if we need to write an image stack (pages > 2).
  this->Pages = extent[5] - extent[4] + 1;

  cout << this->FileName << endl;
  cout << "Width: " << this->Width << endl;
  cout << "Height: " << this->Height << endl;
  cout << "Pages: " << this->Pages << endl;

  // Check the resolution too, assume we store it in metric (as in reader).
  // TODO: Resolution is ignored
  this->XResolution = 10.0 / data->GetSpacing()[0];
  this->YResolution = 10.0 / data->GetSpacing()[1];

  // Open the TIFF
  TIFF* tif = TIFFOpen(this->FileName, "w");
  if(tif == NULL)
    {
    vtkErrorMacro(<< "Could not create output file " << this->FileName);
    this->SetErrorCode(vtkErrorCode::FileFormatError);
    return;
    }
  this->TIFFPtr = tif;

  cout << "Done opening .." << endl;

  // Set mostly default tif tags
  TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, this->TileSize);
  TIFFSetField(tif, TIFFTAG_IMAGELENGTH, this->TileSize);
  TIFFSetField(tif, TIFFTAG_TILEWIDTH, this->TileSize);
  TIFFSetField(tif, TIFFTAG_TILELENGTH, this->TileSize);
  // TIFFSetField(tif, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
  TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, 3); // Ignore alpha
  TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, 8); // Always same from openslide reader
  TIFFSetField(tif, TIFFTAG_PLANARCONFIG, 1);
  TIFFSetField(tif, TIFFTAG_COMPRESSION, 7); // COMPRESSION_JPEG
  // TIFFSetField(tif, TIFFTAG_JPEGQUALITY, this->JPEGQuality);
  TIFFSetField(tif, TIFFTAG_JPEGCOLORMODE, JPEGCOLORMODE_RAW);
  TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB); // Always same for JPEG

  // Make sure that the
  if (tif == NULL)
    {
    vtkErrorMacro("Problem writing header.");
    this->SetErrorCode(vtkErrorCode::FileFormatError);
    return;
    }
  }

void vtkPTIFWriter::ComputeExtentsFromTileName(std::string & tileName, int * ext)
  {
  // Compute the extents from the prefix
  ext[0] = 0;
  ext[2] = 0;

  int step = this->TileSize;
  std::string::reverse_iterator rit;

  for ( rit=tileName.rbegin() ; rit < tileName.rend(); rit++ )
    {
    // Do the adjustments based on the incoming character
    if(*rit == 'q')
      {
      ext[2] += step;
      }
    if(*rit == 'r')
      {
      ext[0] += step;
      ext[2] += step;
      }
    if(*rit == 's')
      {
      ext[2] += step;
      }
    step = step * 2;
    }

  ext[1] = ext[0] + this->TileSize-1;
  ext[3] = ext[2] + this->TileSize-1;
  ext[4] = 0;
  ext[5] = 0;
  }

void vtkPTIFWriter::ProcessTile(const std::string &current_tile)
  {
  // If belongs to base image then get the images
  if(current_tile.length() >= 4)
    {
    cout << "PYRAMID: Got " << current_tile << endl;
    int extents[6];
    // Compute extents
    this->WriteTile(0, 0, extents, 0); // Only extent is useful parameter
    return;
    }

  // Get parents
  ProcessTile(current_tile + 'q');
  ProcessTile(current_tile + 'r');
  ProcessTile(current_tile + 's');
  ProcessTile(current_tile + 't');

  cout << "PYRAMID: Processing " << current_tile << endl;
  }

//----------------------------------------------------------------------------
void vtkPTIFWriter::WriteFile(ofstream *, vtkImageData *data,
                              int extent[6], int*)
{
  // TODO: Add the logic of calling write tile in a loop
  cout << "PYRAMID START" << endl;
  ProcessTile(std::string("t"));
  // tile_stack.push("t");
  // int count = 0;
  // while(!tile_stack.empty() && count < 2)
  //   {
  //     // Looking for current tile
  //     std::string &current_tile = tile_stack.top();
  //     cout << "PYRAMID: " << tile_stack.size() << endl;
  //
  //     // If belongs to base image then get the images
  //     if(current_tile.length() >= 4)
  //       {
  //       cout << "PYRAMID: Got " << current_tile << endl;
  //       tile_stack.pop();
  //       }
  //     else
  //       {
  //       // Get parents
  //       cout << "PYRAMID: Pushing parents of " << current_tile << endl;
  //       tile_stack.push(current_tile + 'q');
  //       tile_stack.push(current_tile + 'r');
  //       tile_stack.push(current_tile + 's');
  //       tile_stack.push(current_tile + 't');
  //       }
  //
  //       cout << "PYRAMID: Processed " << current_tile << endl;
  //       tile_stack.pop();
  //       count ++;
        // Combine
        // newim = Image.new('RGB', (tilesize * 2, tilesize * 2), color=None)
        //
        // # Combine
        // newim.paste(q, (0, 0))
        // newim.paste(r, (tilesize, 0))
        // newim.paste(s, (tilesize, tilesize))
        // newim.paste(t, (0, tilesize))
        //
        // # Resize
        // smallim = newim.resize((tilesize, tilesize), Image.ANTIALIAS)
        //
        // # Compress
        // output = StringIO.StringIO()
        // smallim.save(output, format='JPEG')
        // contents = output.getvalue()
        // output.close()

    // }


  if (this->TIFFPtr == NULL)
    {
    vtkErrorMacro("Problem writing data.");
    this->SetErrorCode(vtkErrorCode::FileFormatError);
    return;
    }
}


void vtkPTIFWriter::WriteTile(ofstream * ignored_stream, vtkImageData *ignored_data,
                              int extent[6], int* ignored_index)
{
  // Set the requested extents
  cout << "NeedExtents: " << extent[0] << ", " << extent[1] << endl;
  vtkStreamingDemandDrivenPipeline* exec = vtkStreamingDemandDrivenPipeline::SafeDownCast(this->GetExecutive());
  exec->SetUpdateExtent(this->GetInputInformation(0, 0), extent);

  // Update the data
  // For debug
  // int *uExtent;
  // uExtent = vtkStreamingDemandDrivenPipeline::GetUpdateExtent(
  //   this->GetInputInformation(0, 0));
  // cout << "UpdateExtents" << uExtent[0] << ", " << uExtent[1] << endl;
  this->Update();

  // Access the image data
  vtkImageData *data = this->GetInput();
  int ex[6];
  data->GetExtent(ex);
  cout << "Data: " << ex[0] << ", " << ex[1] << endl;
  cout << "Extent: " << extent[0] << ", " << extent[1] << endl;

  // for debug
  // vtkNew<vtkJPEGWriter> vtkWr;
  // vtkWr->SetFileName("temp.jpg");
  // vtkWr->SetQuality(70);
  // vtkWr->ProgressiveOff();
  // vtkWr->SetInputData(data);
  // vtkWr->Write();

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

  // Compute tile name
  int tNum = TIFFComputeTile(this->TIFFPtr, 0, 0, 0, 0);
  cout << "TNum: " << tNum << endl;

  // Write out the tile
  void *inPtr = data->GetScalarPointer();
  if (TIFFWriteEncodedTile(this->TIFFPtr, tNum, static_cast<unsigned char*>(inPtr), TIFFTileSize(this->TIFFPtr)) < 0)
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
