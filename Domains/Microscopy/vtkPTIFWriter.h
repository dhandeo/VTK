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

#ifndef vtkPTIFWriter_h
#define vtkPTIFWriter_h

#include "vtkDomainsMicroscopyModule.h" // For export macro
#include "vtkImageWriter.h" // Image writer
#include "vtkImageData.h" // for returning tile data
#include "vtkSmartPointer.h" // For function returning a pointer to imagedata
#include <string>
#include <vector>
extern "C" {
  #include "vtk_tiff.h" // tiff library
}

enum COMPRESSION_MODE
  {
  COMPRESS_WITH_VTK,
  COMPRESS_WITH_JPEGLIB
  };

class VTKDOMAINSMICROSCOPY_EXPORT vtkPTIFWriter : public vtkImageWriter
{
public:
  static vtkPTIFWriter *New();
  vtkTypeMacro(vtkPTIFWriter,vtkImageWriter);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Configurable parameters
  vtkSetVector3Macro(Padding, unsigned char);
  vtkGetVector3Macro(Padding, unsigned char);

  vtkSetMacro(JPEGQuality, int);
  vtkGetMacro(JPEGQuality, int);

  vtkSetMacro(TileSize, int);
  vtkGetMacro(TileSize, int);

  // Description:
  // The main interface which triggers the writer to start.
  virtual void Write();
  void ComputeExtentsFromTileName(const std::string &tileName, int * ext);
  int internalComputeMaxLevel(int, int);

protected:
  vtkPTIFWriter();
  ~vtkPTIFWriter() {}

  // Internal variables
  TIFF* TIFFPtr; // Pointer to tif file opened for read / write
  int CurDir; // Current directory in tiff file
  int DataUpdateExtent[6];
  int DataType;
  int NumScalars;
  int MaxLevel; // Depends on the max extent
  unsigned char CompressionMode;


  // Extents for combinling lower tiles into upper tile
  int qExtent[6];
  int rExtent[6];
  int sExtent[6];
  int tExtent[6];

  std::vector<int> heights;

  //
  int Compression;
  int Width;
  int Height;
  int Pages;
  double XResolution;
  double YResolution;

  // Three parameters that are configurable
  unsigned char Padding[3];
  int JPEGQuality;
  int TileSize;
  int ComputeMaxLevel();
  void InitPyramid();

  virtual int RequestInformation(vtkInformation *request,
                                 vtkInformationVector **inputVector,
                                 vtkInformationVector *outputVector);

  virtual int RequestUpdateExtent(vtkInformation *request,
                                  vtkInformationVector **inputVector,
                                  vtkInformationVector *outputVector);

  virtual int RequestData(vtkInformation* request,
                          vtkInformationVector** inputVector,
                          vtkInformationVector* outputVector);

  virtual void WriteFile(ofstream *file, vtkImageData *data, int ext[6], int wExt[6]);
  virtual void WriteFileHeader(ofstream *, vtkImageData *, int wExt[6]);
  virtual void WriteFileTrailer(ofstream *, vtkImageData *);

  void SelectDirectory(int dir);
  void WriteTile(vtkImageData *data, int *extent, int level);
  vtkSmartPointer<vtkImageData> ProcessTile(const std::string &current_tile);

  void TileDataCompressWithVTK(int num, vtkImageData *data);
  void TileDataCompressWithJPEGLib(int num, vtkImageData *data);

  int IsFullTileWithinImage(int *extents, int *valid_extents);


  vtkPTIFWriter(const vtkPTIFWriter&);  // Not implemented.
  void operator=(const vtkPTIFWriter&);  // Not implemented.
};
#endif
