/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPTIFWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.


  Notes:

  Extents - Usually the requested extents by the pyramid creator
  valid_extents (should be )
  paddngy - Most important the width of padding (useful for logic on where to place data)
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
#include "vtkImageShrink3D.h"
#include "vtkSmartPointer.h"
#include "vtkUnsignedCharArray.h"

#include <stack>
#include <string>
#include <iostream>
#include <algorithm>
#include <assert.h>

#if _MSC_VER
#define snprintf _snprintf
#endif

vtkStandardNewMacro(vtkPTIFWriter);

//----------------------------------------------------------------------------
vtkPTIFWriter::vtkPTIFWriter()
  : TIFFPtr(NULL), Width(0), Height(0), Pages(0), CurDir(0), MaxLevel(0),
    XResolution(-1.0), YResolution(-1.0), JPEGQuality(75), TileSize(256),
    CompressionMode(COMPRESS_WITH_VTK)
{
  // These are now where the tif expects, i.e. left top alignment
  this->ComputeExtentsFromTileName("tq", this->qExtent);
  this->ComputeExtentsFromTileName("tr", this->rExtent);
  this->ComputeExtentsFromTileName("ts", this->sExtent);
  this->ComputeExtentsFromTileName("tt", this->tExtent);

  // cout << "q: "   << this->qExtent[0]
  //         << ", " << this->qExtent[1]
  //         << ", " << this->qExtent[2]
  //         << ", " << this->qExtent[3]  << endl;

  this->SetPadding(255, 255, 255); // White background by default
  this->InitBackgroundTile();
}


void vtkPTIFWriter::SetTileSize(int t)
{
  // TODO: Verify
  this->TileSize = t;
  this->InitBackgroundTile();
}

void vtkPTIFWriter::InitBackgroundTile()
{
    // Initialize white tile
    if(this->background_tile != NULL)
    {
      this->background_tile = NULL; // Calls the destructor
    }

    this->background_tile = vtkImageData::New();
    this->background_tile->SetExtent(0,this->TileSize-1,0, this->TileSize-1, 0,0);
    this->background_tile->AllocateScalars(VTK_UNSIGNED_CHAR, 3);

    for(int j=0; j<this->TileSize; j++)
      {
      for(int k=0; k<this->TileSize; k++)
        {
          for(int m =0; m < this->background_tile->GetNumberOfScalarComponents(); m++)
          {
          this->background_tile->SetScalarComponentFromDouble(j,k,0,m, this->Padding[m]);
          }
        }
      }
}


void vtkPTIFWriter::SetPaddingColorRGB(unsigned char r, unsigned char g, unsigned char b)
{
  this->SetPadding(r, g, b);
  this->InitBackgroundTile();
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
    vtkErrorMacro(<<"Write: Please specify an output FileNamcurrent_tile.length()-1e");
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
  // cout << "DataType: " << hup << " Should be " << VTK_UNSIGNED_CHAR << endl;

  this->DataType = dataType;
  this->NumScalars = components;

  for(int i=0; i < 6; i ++) this->DataUpdateExtent[i] = extent[i];
  // cout << "RequestInformation: " << extent[0] << ", " << extent[1] << endl;

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
  // cout << "RequestUpdateExtent: " << n << "Information  Objects" << endl;
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);

  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
              this->DataUpdateExtent, 6);

  // cout << "RequestUpdateExtent: " << endl;
  this->Modified();
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
    // cout << "## RequestData: " << extents[0] << ", " << extents[1] << ", " << extents[2] <<", " << extents[3] <<endl;
    // cout << "InputDims: " << dim[0] << ", " << dim[1] << endl;

  return 1;
}

void vtkPTIFWriter::SelectDirectory(int dir)
{
  // The logic is moved to init pyramid
}

//----------------------------------------------------------------------------
void vtkPTIFWriter::WriteFileHeader(ofstream *, vtkImageData *data2, int wExt[6])
{
  int dims[3];
  // cout << "InHeader " << endl;
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

  this->MaxLevel = this->ComputeMaxLevel();

  // Check if we need to write an image stack (pages > 2).
  this->Pages = extent[5] - extent[4] + 1;

  // Debug
  // cout << this->FileName << endl;
  // cout << "Width: " << this->Width << endl;
  // cout << "Height: " << this->Height << endl;
  // cout << "MaxLevel: " << this->MaxLevel << endl;

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
  this->InitPyramid();

  // Make sure that the
  if (tif == NULL)
    {
    vtkErrorMacro("Problem writing header.");
    this->SetErrorCode(vtkErrorCode::FileFormatError);
    return;
    }
  }

int vtkPTIFWriter::internalComputeMaxLevel(int width, int height)
  {
  assert(width != 0);
  assert(height != 0);

  float ma = std::max(width, height);
  ma /= this->TileSize;
  return int(ceil(log2(ma)));
  }

int vtkPTIFWriter::ComputeMaxLevel()
  {
  return this->internalComputeMaxLevel(this->Width, this->Height);
  // Compute max level from Width and Height
  }

void debug_jpeg(std::string const tile, std::string prefix, vtkImageData* img)
  {
    vtkNew<vtkJPEGWriter> vtkWr2;
    std::string fname2(tile);
    vtkWr2->SetFileName(fname2.append(prefix).c_str());
    vtkWr2->SetQuality(70);
    vtkWr2->ProgressiveOff();
    vtkWr2->SetInputData(img);
    vtkWr2->Write();
  }

void vtkPTIFWriter::ComputeExtentsFromTileName(const std::string & tileName, int * ext)
  {
  // Compute the extents from the prefix
  // Returned extents are with origin at top, left with y axis looking downwards
  ext[0] = 0;
  ext[2] = 0;

  int step = this->TileSize;
  unsigned long height = this->TileSize;

  for ( std::string::const_reverse_iterator rit=tileName.rbegin() ; rit < tileName.rend(); rit++ )
    {
    height *= 2;

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
      ext[0] += step;
      }
    step = step * 2;
    }

  height /= 2;

  ext[1] = ext[0] + this->TileSize-1;
  ext[3] = ext[2] + this->TileSize-1;
  ext[4] = 0;
  ext[5] = 0;

  // Subtract from the height and swap
  int temp = ext[2];
  ext[2] = height - ext[3]-1;
  ext[3] = height - temp-1;

  }

int vtkPTIFWriter::IsFullTileWithinImage(int *extents, int *valid_extents, int width, int height)
  {
  // Returns true if the request extents are within whole extents of the image, false otherwise
  // valid_extents returned reflect the imagedata that is within whole_extents of the image
  // If the image is not partial and in the image then the valid_extents are ignored

  // assumes that extents[0] < extents[1]

  int partial = WITHIN;

  assert(extents[0] >= 0);
  // assert(extents[2] >= 0);

  if(extents[0] >= width || extents[2] >= height)
    {
    return OUTSIDE; // Dont bother updating valid extents
    }

  // highest extent allowed is width-1
  if(extents[1] >= width)
    {
    valid_extents[1] = width - 1;
    valid_extents[2] = extents[2];
    partial = PARTIAL;
    // cout << "XPARTIAL" << endl;
    }

  // repeat same for height
  if(extents[2] <= 0)
    {
    // Clamp the height
    valid_extents[2] = 0;
    // If x is already partial then valid_extents[1] is already clamped
    if(partial != PARTIAL)
      {
      valid_extents[1] = extents[1];
      partial = PARTIAL;
      }
    }

  if(partial != WITHIN)
    {
    // Make sure valid_extents are filled
    valid_extents[0] = extents[0];
    valid_extents[3] = extents[3];
    valid_extents[4] = extents[4];
    valid_extents[5] = extents[5];
    }

  return partial;
  }

vtkSmartPointer<vtkImageData> vtkPTIFWriter::ProcessTile(const std::string &current_tile)
{
  // Processes tile, by requesting and merging children of the pyramid tree.
  // Returns image read from the file if no further children

  int extents[6];
  int height;
  int valid_extents[6];
  this->ComputeExtentsFromTileName(current_tile, extents);   // Extents are used later only for returning the final tile

  // cout << "    OExtents: " << extents[0]  << ", " << extents[1]  << ", " << extents[2] << ", " << extents[3]  << endl;

  // Level to which to write the image
  // Level 0 is full resolution and last level depends on the
  int level = this->MaxLevel - current_tile.length()+1;

  // Compute the extents for vtk coordinate system for reading from the image
  // TODO: DJ possibly not required
  this->DataUpdateExtent[0] = extents[0];
  this->DataUpdateExtent[1] = extents[1];
  this->DataUpdateExtent[2] = this->heights[level]-1 - extents[3];
  this->DataUpdateExtent[3] = this->heights[level]-1 - extents[2];
  this->DataUpdateExtent[4] = extents[4];
  this->DataUpdateExtent[5] = extents[5];
  // Here DataUpdateExtent can be negative but that only means padding is required in the bottom

  // cout << "    Dataext: " << DataUpdateExtent[0]  << ", " << DataUpdateExtent[1]  << ", " << DataUpdateExtent[2] << ", " << DataUpdateExtent[3]  << endl;
  int tile_status = this->IsFullTileWithinImage(this->DataUpdateExtent, valid_extents, this->widths[level], this->heights[level]);

  if(tile_status == OUTSIDE)
    {
    vtkSmartPointer<vtkImageData> ret = vtkImageData::New();
    ret->DeepCopy(this->background_tile);
    return ret; // Should never be called and can be pruned
    }

    // Debug partial tile
    // if(tile_status == PARTIAL)
    //   {
    //   cout << "  PARTIAL" << endl;
    //   cout << "    Valid  : "   << valid_extents[0]
    //                     << ", " << valid_extents[1]
    //                     << ", " << valid_extents[2]
    //                     << ", " << valid_extents[3]  << endl;
    //   }


  // If belongs to base image then get the images
  if(current_tile.length() >= this->MaxLevel + 1)
    {
    // cout << "PYRAMID: Input " << current_tile << endl;
    // Get data from input

    //TODO: DJ find out what extents are availabel in file
    vtkStreamingDemandDrivenPipeline* exec = vtkStreamingDemandDrivenPipeline::SafeDownCast(this->GetExecutive());

    if (tile_status == PARTIAL)
      {
      // cout << "  PARTIAL" << endl;
      this->DataUpdateExtent[0] = valid_extents[0];
      this->DataUpdateExtent[1] = valid_extents[1];
      this->DataUpdateExtent[2] = valid_extents[2];
      this->DataUpdateExtent[3] = valid_extents[3];
      this->DataUpdateExtent[4] = valid_extents[4];
      this->DataUpdateExtent[5] = valid_extents[5];
      exec->SetUpdateExtent(this->GetInputInformation(0, 0), this->DataUpdateExtent);
      }
    else
      {
      // cout << "  WITHIN" << endl;
      this->DataUpdateExtent[0] = extents[0];
      this->DataUpdateExtent[1] = extents[1];
      this->DataUpdateExtent[2] = this->heights[level]-1 - extents[3];
      this->DataUpdateExtent[3] = this->heights[level]-1 - extents[2];
      this->DataUpdateExtent[4] = extents[4];
      this->DataUpdateExtent[5] = extents[5];
      exec->SetUpdateExtent(this->GetInputInformation(0, 0), this->DataUpdateExtent);
      }

    this->Modified(); // TODO:DJ Verify that this is not making the streaming slow
    this->Update();


    // Prepare the return data
    vtkSmartPointer<vtkImageData> ret = vtkImageData::New();
    // TODO:DJ Copy entire data only if partial tile
    ret->DeepCopy(this->background_tile);

    if(tile_status == PARTIAL)
      {
      // Where to pad
      int imageextents[6];
      imageextents[4] = 0;
      imageextents[5] = 0;

      // Compose image extents (i.e. where the image should go) with bottom and right padding
      int xwidth = this->DataUpdateExtent[1]-this->DataUpdateExtent[0];
      int ywidth = this->DataUpdateExtent[3]-this->DataUpdateExtent[2]; // Could be zero

      imageextents[0] = 0;
      imageextents[1] = this->DataUpdateExtent[1]-this->DataUpdateExtent[0];
      imageextents[2] = this->TileSize-1 - ywidth;
      imageextents[3] = this->TileSize-1;

      vtkNew<vtkImageData> temp;
      temp->DeepCopy(this->GetInput());
      temp->SetExtent(imageextents);

      // Debug
      // cout << "imageexq: "    << imageextents[0]
      //                 << ", " << imageextents[1]
      //                 << ", " << imageextents[2]
      //                 << ", " << imageextents[3]  << endl;

      ret->SetExtent(0, this->TileSize -1, 0, this->TileSize -1, 0, 0);
      ret->CopyAndCastFrom(temp.GetPointer(), imageextents);
      }
    else
      {
      // TODO:DJ initialize the imagedata here
      ret->SetExtent(this->DataUpdateExtent);
      ret->CopyAndCastFrom(this->GetInput(), this->DataUpdateExtent);
      // ret->Print(cout);
      }
    // Output
    ret->SetExtent(extents);
    this->WriteTile(ret, extents, level); // Only extent is useful parameter

    // Optionally write intermediate output
    // debug_jpeg(current_tile, std::string("_ready.jpg"), ret);
    return ret;
    }

  // Get parents
  vtkSmartPointer<vtkImageData> t = ProcessTile(current_tile + 't');
  vtkSmartPointer<vtkImageData> s = ProcessTile(current_tile + 's');
  vtkSmartPointer<vtkImageData> r = ProcessTile(current_tile + 'r');
  vtkSmartPointer<vtkImageData> q = ProcessTile(current_tile + 'q');

  // Optionally write out what was returned from the lower level of pyramid
  // debug_jpeg(current_tile, std::string("_q_really.jpg"), q.GetPointer());
  // debug_jpeg(current_tile, std::string("_t_really.jpg"), t.GetPointer());

  // Combine
  vtkNew<vtkImageData> big;
  int bigdim = this->TileSize*2;

  big->SetDimensions(bigdim, bigdim, 1);
  big->SetExtent(0,bigdim-1,0,bigdim-1,0,0);
  big->AllocateScalars(VTK_UNSIGNED_CHAR, 3);

  // Explicit Padding
  // void *ptr = big->GetScalarPointer();
  // memset(ptr, this->Padding[0], bigdim*bigdim*3);
  // debug_jpeg(current_tile, std::string("_big_blank.jpg"), big.GetPointer());

  q->SetExtent(this->tExtent); big->CopyAndCastFrom(q, this->tExtent);
  r->SetExtent(this->sExtent); big->CopyAndCastFrom(r, this->sExtent);
  s->SetExtent(this->rExtent); big->CopyAndCastFrom(s, this->rExtent);
  t->SetExtent(this->qExtent); big->CopyAndCastFrom(t, this->qExtent);

  // Shrink
  vtkNew<vtkImageShrink3D> shrinkFilter;
  shrinkFilter->SetInputData(big.GetPointer());
  shrinkFilter->SetShrinkFactors(2,2,1);
  shrinkFilter->Update();

  // Write it out
  // debug_jpeg(current_tile, std::string("_ready.jpg"), shrinkFilter->GetOutput());
  this->WriteTile(shrinkFilter->GetOutput(), extents, level);
  // cout << "PYRAMID: Processed: " << current_tile << endl;

  vtkSmartPointer<vtkImageData> ret=vtkImageData::New();
  ret->ShallowCopy(shrinkFilter->GetOutput());
  return ret;
  }

//----------------------------------------------------------------------------
void vtkPTIFWriter::InitPyramid()
{
  // Loops through all the levels after the Width and Height and tilesize have been set
  // Expects tiff to be open and maxlevel to be computed

  int width = this->Width;
  int height = this->Height;
  TIFF *tif = this->TIFFPtr;

  this->heights.reserve(this->MaxLevel + 1);
  this->widths.reserve(this->MaxLevel + 1);

  for (int level = 0; level <= this->MaxLevel; level++)
    {
    // cout << "INIT Level: " << level << ": " << width << ", " << height << endl;
    this->heights[level] = height;
    this->widths[level] = width;

    // Set mostly default tif tags
    // We are writing single page of the multipage file
    TIFFSetField(tif, TIFFTAG_SUBFILETYPE, FILETYPE_PAGE);
    TIFFSetField(tif, TIFFTAG_PAGENUMBER, level, this->MaxLevel + 1);

    TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, width);
    TIFFSetField(tif, TIFFTAG_IMAGELENGTH, height);
    TIFFSetField(tif, TIFFTAG_TILEWIDTH, this->TileSize);
    TIFFSetField(tif, TIFFTAG_TILELENGTH, this->TileSize);

    // Not all readers will support orientation bottomleft
    // TIFFSetField(tif, TIFFTAG_ORIENTATION, ORIENTATION_BOTLEFT);
    TIFFSetField(tif, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);

    TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, 3); // Ignore alpha
    TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, 8); // Always same from openslide reader
    TIFFSetField(tif, TIFFTAG_PLANARCONFIG, 1);

    // TIFFSetField(tif, TIFFTAG_JPEGTABLESMODE, 0); // Always same for JPEG
    TIFFSetField(tif, TIFFTAG_COMPRESSION, 7); // COMPRESSION_JPEG
    TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_YCBCR); // Always same for JPEG
    TIFFSetField(tif, TIFFTAG_JPEGCOLORMODE, JPEGCOLORMODE_RGB);
    // TIFFSetField(tif, TIFFTAG_JPEGQUALITY, this->JPEGQuality);

    // cout << "   Numtiles: " << TIFFNumberOfTiles(tif) << endl;
    TIFFCheckpointDirectory(this->TIFFPtr);
    TIFFWriteDirectory(this->TIFFPtr);

    // Get next image size
    width = this->TileSize * int((ceil(float(width) / (2 * this->TileSize))));
    height = this->TileSize * int((ceil(float(height) / (2 * this->TileSize))));
    }
}


//----------------------------------------------------------------------------
void vtkPTIFWriter::WriteFile(ofstream *file, vtkImageData *data, int ext[6], int wExt[6])
{
  if (this->TIFFPtr == NULL)
    {
    vtkErrorMacro("Problem writing data.");
    this->SetErrorCode(vtkErrorCode::FileFormatError);
    return;
    }

  // TODO: Multithreading requests should split the requests here

  // Recursively build pyramid
  vtkImageData *t = ProcessTile(std::string("t"));
}


void vtkPTIFWriter::WriteTile(vtkImageData *data, int *extent, int level)
{
  // Write imagedata in the tiff tile where requested
  // Set the requested extents

  // Access the image data
  // int ex[6];
  // data->GetExtent(ex);
  // cout << "  Data: "    << ex[0]      << ", " << ex[1]      << ", " <<  ex[2]     << ", " << ex[3]      << endl;

  // Make sure tiles in the other directory are valid
  TIFFCheckpointDirectory(this->TIFFPtr);
  TIFFSetDirectory(this->TIFFPtr, level);
  // cout << "  " << level << "], " <<  "Extent: "  << extent[0]  << ", " << extent[1]  << ", " << extent[2] << ", " << extent[3]  << endl;
  // cout << "  Writing to: " << TIFFCurrentDirectory(this->TIFFPtr);

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
  int tNum = TIFFComputeTile(this->TIFFPtr, extent[0], extent[2], 0, 0);
  // cout << ", " << tNum << endl;

  if(this->CompressionMode == COMPRESS_WITH_VTK)
    {
    this->TileDataCompressWithVTK(tNum, data);
    }
  else
    {
    // This mode is not supported
    this->TileDataCompressWithJPEGLib(tNum, data);
    }
}

void vtkPTIFWriter::TileDataCompressWithJPEGLib(int num, vtkImageData *data)
{
  // Not supported
  // Comment out following line first before developing

  // TODO:DJ Need to convert image buffer to ycbcr and downsample
  // it before the jpeg compression is called
  vtkErrorMacro(<< "Compresison with JPEGLib is not supported in this version");

  void *inPtr = data->GetScalarPointer();
  if (TIFFWriteEncodedTile(this->TIFFPtr, num, static_cast<unsigned char*>(inPtr), TIFFTileSize(this->TIFFPtr)) < 0)
    {
    vtkErrorMacro(<< "Error writing tile");
    this->SetErrorCode(vtkErrorCode::FileFormatError);
    }
}



void vtkPTIFWriter::TileDataCompressWithVTK(int num, vtkImageData *data)
{
    // Temporary memory used to hold compressed tile.
    vtkUnsignedCharArray* jpgData;
    vtkTypeUInt32 compressedTileSize;
    // Writer used to compress the tile.
    vtkSmartPointer<vtkJPEGWriter> vtkW = vtkSmartPointer<vtkJPEGWriter>::New();
    vtkW->WriteToMemoryOn();
    vtkW->SetFilePattern("Tile_%d"); // Ignored as writing to memory
    vtkW->SetInputData(data);
    vtkW->SetQuality(this->JPEGQuality);
    vtkW->ProgressiveOff();
    vtkW->Write();
    jpgData = vtkW->GetResult();
    compressedTileSize = jpgData->GetNumberOfTuples();

    // TODO:DJ Trim the jpegtables

    // Drop
    // ff:e0 JFIF optional
    // ff:c0 Missing in the tiles generated by vips optional base dct table

    // jpegtables
    // 0]  ff:d8
    // 2]  ff:db
    // 71]  ff:db
    // 140]  ff:c4
    // 173]  ff:c4
    // 356]  ff:c4
    // 389]  ff:c4
    // 572]  ff:d9

    // raw tiles
    // 0]  ff:d8
    // 2]  ff:db
    // 71]  ff:db
    // 140]  ff:c0
    // 159]  ff:da
    // 173]  f9:d5
    // 13810]  8c:84

    // Output debug image
    // std::ofstream ofile("test.jpg", ios::out | ios::binary);
    // ofile.write((const char *) jpgData->GetVoidPointer(0), compressedTileSize);

    // cout << "   Compressed TileSize: " << compressedTileSize << endl;
    if (TIFFWriteRawTile(this->TIFFPtr, num, (unsigned char *) jpgData->GetVoidPointer(0), compressedTileSize) < 0)
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
}
