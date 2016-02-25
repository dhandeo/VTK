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
#include <assert.h>

// VTK includes
#include <vtkTestUtilities.h>

// Main program
int TestPTIFWriterExtra(int argc, char** argv)
{
  vtkNew<vtkPTIFWriter> writer;
  int extents[6];
  writer->ComputeExtentsFromTileName(std::string("t"), extents);
  cout << "t: " << extents[0] << ", " << extents[1] << ", "  << extents[2] << ", "  << extents[3] << endl;
  writer->ComputeExtentsFromTileName(std::string("tq"), extents);
  cout << "tq: " << extents[0] << ", " << extents[1] << ", "  << extents[2] << ", "  << extents[3] << endl;
  writer->ComputeExtentsFromTileName(std::string("tr"), extents);
  cout << "tr: " << extents[0] << ", " << extents[1] << ", "  << extents[2] << ", "  << extents[3] << endl;
  writer->ComputeExtentsFromTileName(std::string("ts"), extents);
  cout << "ts: " << extents[0] << ", " << extents[1] << ", "  << extents[2] << ", "  << extents[3] << endl;
  writer->ComputeExtentsFromTileName(std::string("tt"), extents);
  cout << "tt: " << extents[0] << ", " << extents[1] << ", "  << extents[2] << ", "  << extents[3] << endl;
  writer->ComputeExtentsFromTileName(std::string("trq"), extents);
  cout << "trq: " << extents[0] << ", " << extents[1] << ", "  << extents[2] << ", "  << extents[3] << endl;
  writer->ComputeExtentsFromTileName(std::string("trr"), extents);
  cout << "trr: " << extents[0] << ", " << extents[1] << ", "  << extents[2] << ", "  << extents[3] << endl;
  writer->ComputeExtentsFromTileName(std::string("trs"), extents);
  cout << "trs: " << extents[0] << ", " << extents[1] << ", "  << extents[2] << ", "  << extents[3] << endl;
  writer->ComputeExtentsFromTileName(std::string("trt"), extents);
  cout << "trt: " << extents[0] << ", " << extents[1] << ", "  << extents[2] << ", "  << extents[3] << endl;

  cout << endl;

  assert(0 == writer->internalComputeMaxLevel(200,200));
  assert(2 == writer->internalComputeMaxLevel(200,513));
  assert(3 == writer->internalComputeMaxLevel(1025,513));
  assert(3 == writer->internalComputeMaxLevel(2048,1024));

  cout << "Total levels for 200,200: " << writer->internalComputeMaxLevel(200,200) + 1 << endl;
  cout << "Total levels for 200,513: " << writer->internalComputeMaxLevel(200,513) + 1 << endl;
  cout << "Total levels for 1025,513: " << writer->internalComputeMaxLevel(1025,513) + 1 << endl;
  cout << "Total levels for 2048,1024: " << writer->internalComputeMaxLevel(2048,1024) + 1 << endl;

  return EXIT_SUCCESS;
}
