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
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkImageViewer2.h>
#include <vtkImageData.h>
#include <vtkPNGWriter.h>

// VTK includes
#include <vtkTestUtilities.h>

// C++ includes
#include <sstream>

// Main program
int TestOpenSlideReaderPartial(int argc, char** argv)
{
  const char* rasterFileName = vtkTestUtilities::ExpandDataFileName(argc, argv,
                                 "Data/Microscopy/small2.ndpi");

  //std::cout << "Got Filename: " << rasterFileName << std::endl;

  // Create reader to read shape file.
  vtkNew<vtkOpenSlideReader> reader;
  reader->SetFileName(rasterFileName);
  reader->UpdateInformation();
  reader->Print(cout);
  delete [] rasterFileName;

  int extent[6] = {20,120,20,120,0,0};
  reader->SetUpdateExtent(extent);
  reader->Update();
  reader->Print(cout);

  vtkNew<vtkImageData> data;
  data->ShallowCopy(reader->GetOutput());

  vtkNew<vtkPNGWriter> writer;
  writer->SetInputData(data.GetPointer());
  writer->SetFileName("this.png");
  writer->SetUpdateExtent(extent);
  writer->Update();
  writer->Write();

  int extent2[6] = {200,499,200,499,0,0};

  reader->SetUpdateExtent(extent2);
  reader->Update();
  reader->Print(cout);

  data->ShallowCopy(reader->GetOutput());

  vtkNew<vtkPNGWriter> write;
  write->SetInputData(data.GetPointer());
  write->SetFileName("this2.png");
  write->SetUpdateExtent(extent);
  write->Update();
  write->Write();




  // // Visualize
  // vtkNew<vtkRenderer> renderer;
  // vtkNew<vtkRenderWindow> window;
  // window->AddRenderer(renderer.GetPointer());
  //
  // vtkNew<vtkRenderWindowInteractor> renderWindowInteractor;
  // renderWindowInteractor->SetRenderWindow(window.GetPointer());
  //
  // vtkNew<vtkImageViewer2> imageViewer;
  // imageViewer->SetInputConnection(reader->GetOutputPort());
  // //imageViewer->SetExtent(1000,1500,1000,1500,0,0);
  // imageViewer->SetupInteractor(renderWindowInteractor.GetPointer());
  // //imageViewer->SetSlice(0);
  // imageViewer->Render();
  // imageViewer->GetRenderer()->ResetCamera();
  // renderWindowInteractor->Initialize();
  // imageViewer->Render();
  // renderWindowInteractor->Start();

  return EXIT_SUCCESS;
}
