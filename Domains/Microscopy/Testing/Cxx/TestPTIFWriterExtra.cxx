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
#include <vtkPTIFWriter.h>
#include <string>

// VTK includes
#include <vtkTestUtilities.h>

// Main program
int TestPTIFWriterExtra(int argc, char** argv)
{
  vtkNew<vtkPTIFWriter> writer;
  int extents[6];
  writer->ComputeExtentsFromTileName(std::string("t"), extents);
  cout << "t: " << extents[0] << ", " << extents[1] << ", "  << extents[2] << ", "  << extents[3] << endl;
  writer->ComputeExtentsFromTileName(std::string("ttq"), extents);
  cout << "ttq: " << extents[0] << ", " << extents[1] << ", "  << extents[2] << ", "  << extents[3] << endl;

  return EXIT_SUCCESS;
}
