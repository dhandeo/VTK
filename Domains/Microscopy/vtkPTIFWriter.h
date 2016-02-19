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

extern "C" {
  #include "vtk_tiff.h" // tiff library
}

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

protected:
  vtkPTIFWriter();
  ~vtkPTIFWriter() {}

  // Internal variables
  TIFF* TIFFPtr;
  int DataUpdateExtent[6];

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

  virtual void WriteTile(ofstream *, vtkImageData *data, int extent[6], int*);


  vtkPTIFWriter(const vtkPTIFWriter&);  // Not implemented.
  void operator=(const vtkPTIFWriter&);  // Not implemented.
};
#endif
