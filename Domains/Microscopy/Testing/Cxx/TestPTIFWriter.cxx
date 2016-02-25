/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestOpenSlideReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include <vtkNew.h>
#include <vtkOpenSlideReader.h>
#include <vtkPTIFWriter.h>

#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkImageViewer2.h>
#include <vtkImageData.h>
#include <vtkJPEGReader.h>
#include <vtkPNGReader.h>

// VTK includes
#include <vtkTestUtilities.h>

// C++ includes
#include <sstream>

// Main program
int TestPTIFWriter(int argc, char** argv)
{
  // const char* rasterFileName = vtkTestUtilities::ExpandDataFileName(argc, argv,
  //                                "Data/Microscopy/small2.ndpi");
  //
  // // const char* rasterFileName = vtkTestUtilities::ExpandDataFileName(argc, argv,
  // //                                "Data/usa_image.jpg");
  //
  // std::cout << "Got Filename: " << rasterFileName << std::endl;
  //
  // // Create reader to read shape file.
  // vtkNew<vtkOpenSlideReader> reader;

  const char* rasterFileName = "/home/dhan/Downloads/castle.jpg";
  std::cout << "Got Filename: " << rasterFileName << std::endl;

  // Create reader to read shape file.
  vtkNew<vtkJPEGReader> reader;

  reader->SetFileName(rasterFileName);
  reader->UpdateInformation();
  // reader->Print(std::cout);
  // delete [] rasterFileName;

  vtkNew<vtkPTIFWriter> writer;
  writer->SetInputConnection(reader->GetOutputPort());
  writer->SetFileName("output.tif");
  writer->Write();

  return EXIT_SUCCESS;
}
