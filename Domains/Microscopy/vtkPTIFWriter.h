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

#include "vtkImageWriter.h"

class VTKIOIMAGE_EXPORT vtkPTIFWriter : public vtkImageWriter
{
public:
  static vtkPTIFWriter *New();
  vtkTypeMacro(vtkPTIFWriter,vtkImageWriter);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // The main interface which triggers the writer to start.
  virtual void Write();

protected:
  vtkPTIFWriter();
  ~vtkPTIFWriter() {}

  void* TIFFPtr;
  int Compression;
  int Width;
  int Height;
  int Pages;
  double XResolution;
  double YResolution;

  virtual void WriteFile(ofstream *file, vtkImageData *data, int ext[6], int wExt[6]);
  virtual void WriteFileHeader(ofstream *, vtkImageData *, int wExt[6]);
  virtual void WriteFileTrailer(ofstream *, vtkImageData *);

  vtkPTIFWriter(const vtkPTIFWriter&);  // Not implemented.
  void operator=(const vtkPTIFWriter&);  // Not implemented.
};
#endif
