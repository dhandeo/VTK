/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointInterpolator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPointInterpolator.h"

#include "vtkObjectFactory.h"
#include "vtkLinearKernel.h"
#include "vtkAbstractPointLocator.h"
#include "vtkStaticPointLocator.h"
#include "vtkDataSet.h"
#include "vtkDataArray.h"
#include "vtkImageData.h"
#include "vtkPoints.h"
#include "vtkCharArray.h"
#include "vtkDoubleArray.h"
#include "vtkCellData.h"
#include "vtkPointData.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkMath.h"
#include "vtkSMPTools.h"
#include "vtkSMPThreadLocalObject.h"

#include <vector>

vtkStandardNewMacro(vtkPointInterpolator);
vtkCxxSetObjectMacro(vtkPointInterpolator,Locator,vtkAbstractPointLocator);
vtkCxxSetObjectMacro(vtkPointInterpolator,Kernel,vtkInterpolationKernel);

//----------------------------------------------------------------------------
// Helper classes to support efficient computing, and threaded execution.
namespace {

#include "vtkArrayListTemplate.h"

// The threaded core of the algorithm
struct ProbePoints
{
  vtkDataSet *Input;
  vtkInterpolationKernel *Kernel;
  vtkAbstractPointLocator *Locator;
  vtkPointData *InPD;
  vtkPointData *OutPD;
  ArrayList Arrays;
  char *Valid;
  int Strategy;

  // Don't want to allocate these working arrays on every thread invocation,
  // so make them thread local.
  vtkSMPThreadLocalObject<vtkIdList> PIds;
  vtkSMPThreadLocalObject<vtkDoubleArray> Weights;

  ProbePoints(vtkDataSet *input, vtkInterpolationKernel *kernel,vtkAbstractPointLocator *loc,
              vtkPointData *inPD, vtkPointData *outPD, int strategy, char *valid, double nullV) :
    Input(input), Kernel(kernel), Locator(loc), InPD(inPD), OutPD(outPD),
    Valid(valid), Strategy(strategy)
    {
      this->Arrays.AddArrays(input->GetNumberOfPoints(), inPD, outPD, nullV);
    }

  // Just allocate a little bit of memory to get started.
  void Initialize()
    {
    vtkIdList*& pIds = this->PIds.Local();
    pIds->Allocate(128); //allocate some memory
    vtkDoubleArray*& weights = this->Weights.Local();
    weights->Allocate(128);
    }

  // When null point is encountered
  void AssignNullPoint(const double x[3], vtkIdList *pIds,
                       vtkDoubleArray *weights, vtkIdType ptId)
    {
      if ( this->Strategy == vtkPointInterpolator::MASK_POINTS)
        {
        this->Valid[ptId] = 0;
        this->Arrays.AssignNullValue(ptId);
        }
      else if ( this->Strategy == vtkPointInterpolator::NULL_VALUE)
        {
        this->Arrays.AssignNullValue(ptId);
        }
      else //vtkPointInterpolator::CLOSEST_POINT:
        {
        pIds->SetNumberOfIds(1);
        vtkIdType pId = this->Locator->FindClosestPoint(x);
        pIds->SetId(0,pId);
        weights->SetNumberOfTuples(1);
        weights->SetValue(0,1.0);
        this->Arrays.Interpolate(1, pIds->GetPointer(0),
                                 weights->GetPointer(0), ptId);
        }
    }

  // Threaded interpolation method
  void operator() (vtkIdType ptId, vtkIdType endPtId)
    {
      double x[3];
      vtkIdList*& pIds = this->PIds.Local();
      vtkIdType numWeights;
      vtkDoubleArray*& weights = this->Weights.Local();

      for ( ; ptId < endPtId; ++ptId)
        {
        this->Input->GetPoint(ptId,x);

        if ( this->Kernel->ComputeBasis(x, pIds) > 0 )
          {
          numWeights = this->Kernel->ComputeWeights(x, pIds, weights);
          this->Arrays.Interpolate(numWeights, pIds->GetPointer(0),
                                   weights->GetPointer(0), ptId);
          }
        else
          {
          this->AssignNullPoint(x, pIds, weights, ptId);
          }// null point
        }//for all dataset points
    }

  void Reduce()
    {
    }

}; //ProbePoints

// Probe points using an image. Uses a more efficient iteration scheme.
struct ImageProbePoints : public ProbePoints
{
  int Dims[3];
  double Origin[3];
  double Spacing[3];

  ImageProbePoints(vtkImageData *image, int dims[3], double origin[3],
                   double spacing[3], vtkInterpolationKernel *kernel,
                   vtkAbstractPointLocator *loc, vtkPointData *inPD,
                   vtkPointData *outPD, int strategy, char *valid, double nullV) :
    ProbePoints(image, kernel, loc, inPD, outPD, strategy, valid, nullV)
    {
      for (int i=0; i < 3; ++i)
        {
        this->Dims[i] = dims[i];
        this->Origin[i] = origin[i];
        this->Spacing[i] = spacing[i];
        }
    }

  // Threaded interpolation method specialized to image traversal
  void operator() (vtkIdType slice, vtkIdType sliceEnd)
    {
      double x[3];
      vtkIdType numWeights;
      double *origin=this->Origin;
      double *spacing=this->Spacing;
      int *dims=this->Dims;
      vtkIdType ptId, jOffset, kOffset, sliceSize=dims[0]*dims[1];
      vtkIdList*& pIds = this->PIds.Local();
      vtkDoubleArray*& weights = this->Weights.Local();

      for ( ; slice < sliceEnd; ++slice)
        {
        x[2] = origin[2] + slice*spacing[2];
        kOffset = slice*sliceSize;

        for ( int j=0;  j < dims[1]; ++j)
          {
          x[1] = origin[1] + j*spacing[1];
          jOffset = j*dims[0];

          for ( int i=0; i < dims[0]; ++i)
            {
            x[0] = origin[0] + i*spacing[0];
            ptId = i + jOffset + kOffset;

            if ( this->Kernel->ComputeBasis(x, pIds) > 0 )
              {
              numWeights = this->Kernel->ComputeWeights(x, pIds, weights);
              this->Arrays.Interpolate(numWeights, pIds->GetPointer(0),
                                       weights->GetPointer(0), ptId);
              }
            else
              {
              this->AssignNullPoint(x, pIds, weights, ptId);
              }// null point

            }//over i
          }//over j
        }//over slices
    }
}; //ImageProbePoints

} //anonymous namespace

//================= Begin class proper =======================================
//----------------------------------------------------------------------------
vtkPointInterpolator::vtkPointInterpolator()
{
  this->SetNumberOfInputPorts(2);

  this->Locator = vtkStaticPointLocator::New();

  this->Kernel = vtkLinearKernel::New();

  this->NullPointsStrategy = vtkPointInterpolator::NULL_VALUE;
  this->NullValue = 0.0;

  this->ValidPointsMask = NULL;
  this->ValidPointsMaskArrayName = "vtkValidPointMask";

  this->PassPointArrays = true;
  this->PassCellArrays = true;
  this->PassFieldArrays = true;
}

//----------------------------------------------------------------------------
vtkPointInterpolator::~vtkPointInterpolator()
{
  this->SetLocator(NULL);
  this->SetKernel(NULL);
}

//----------------------------------------------------------------------------
void vtkPointInterpolator::SetSourceConnection(vtkAlgorithmOutput* algOutput)
{
  this->SetInputConnection(1, algOutput);
}

//----------------------------------------------------------------------------
void vtkPointInterpolator::SetSourceData(vtkDataObject *input)
{
  this->SetInputData(1, input);
}

//----------------------------------------------------------------------------
vtkDataObject *vtkPointInterpolator::GetSource()
{
  if (this->GetNumberOfInputConnections(1) < 1)
    {
    return NULL;
    }

  return this->GetExecutive()->GetInputData(1, 0);
}

//----------------------------------------------------------------------------
void vtkPointInterpolator::
ExtractImageDescription(vtkImageData *input, int dims[3], double origin[3],
                        double spacing[3])
{
  input->GetDimensions(dims);
  input->GetOrigin(origin);
  input->GetSpacing(spacing);
}

//----------------------------------------------------------------------------
// The driver of the algorithm
void vtkPointInterpolator::
Probe(vtkDataSet *input, vtkDataSet *source, vtkDataSet *output)
{
  // Make sure there is a kernel
  if ( !this->Kernel )
    {
    vtkErrorMacro(<<"Interpolation kernel required\n");
    return;
    }

  // Start by building the locator
  if ( !this->Locator )
    {
    vtkErrorMacro(<<"Point locator required\n");
    return;
    }
  this->Locator->SetDataSet(source);
  this->Locator->BuildLocator();

  // Set up the interpolation process
  vtkIdType numPts = input->GetNumberOfPoints();
  vtkPointData *inPD = source->GetPointData();
  vtkPointData *outPD = output->GetPointData();
  outPD->InterpolateAllocate(inPD,numPts);

  // Masking if requested
  char *mask=NULL;
  if ( this->NullPointsStrategy == vtkPointInterpolator::MASK_POINTS )
    {
    this->ValidPointsMask = vtkCharArray::New();
    this->ValidPointsMask->SetNumberOfTuples(numPts);
    mask = this->ValidPointsMask->GetPointer(0);
    std::fill_n(mask, numPts, 1);
    }

  // Now loop over input points, finding closest points and invoking kernel.
  if ( this->Kernel->GetRequiresInitialization() )
    {
    this->Kernel->Initialize(this->Locator, source, inPD);
    }

  // If the input is image data then there is a faster path
  vtkImageData *imgInput = vtkImageData::SafeDownCast(input);
  if ( imgInput )
    {
    int dims[3];
    double origin[3], spacing[3];
    this->ExtractImageDescription(imgInput,dims,origin,spacing);
    ImageProbePoints imageProbe(imgInput, dims, origin, spacing,
                                this->Kernel,this->Locator,inPD,outPD,
                                this->NullPointsStrategy,mask,this->NullValue);
    vtkSMPTools::For(0, dims[2], imageProbe);//over slices
    }
  else
    {
    ProbePoints probe(input,this->Kernel,this->Locator,inPD,outPD,
                      this->NullPointsStrategy,mask,this->NullValue);
    vtkSMPTools::For(0, numPts, probe);
    }

  // Clean up
  if ( mask )
    {
    this->ValidPointsMask->SetName(this->ValidPointsMaskArrayName);
    outPD->AddArray(this->ValidPointsMask);
    this->ValidPointsMask->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkPointInterpolator::
PassAttributeData(vtkDataSet* input, vtkDataObject* vtkNotUsed(source),
                  vtkDataSet* output)
{
  // copy point data arrays
  if (this->PassPointArrays)
    {
    int numPtArrays = input->GetPointData()->GetNumberOfArrays();
    for (int i=0; i<numPtArrays; ++i)
      {
      output->GetPointData()->AddArray(input->GetPointData()->GetArray(i));
      }
    }

  // copy cell data arrays
  if (this->PassCellArrays)
    {
    int numCellArrays = input->GetCellData()->GetNumberOfArrays();
    for (int i=0; i<numCellArrays; ++i)
      {
      output->GetCellData()->AddArray(input->GetCellData()->GetArray(i));
      }
    }

  if (this->PassFieldArrays)
    {
    // nothing to do, vtkDemandDrivenPipeline takes care of that.
    }
  else
    {
    output->GetFieldData()->Initialize();
    }
}

//----------------------------------------------------------------------------
int vtkPointInterpolator::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *sourceInfo = inputVector[1]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkDataSet *input = vtkDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataSet *source = vtkDataSet::SafeDownCast(
    sourceInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataSet *output = vtkDataSet::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  if (!source)
    {
    return 0;
    }

  // Copy the input geometry and topology to the output
  output->CopyStructure(input);

  // Perform the probing
  this->Probe(input, source, output);

  // Pass attribute data as requested
  this->PassAttributeData(input, source, output);

  return 1;
}

//----------------------------------------------------------------------------
int vtkPointInterpolator::RequestInformation(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *sourceInfo = inputVector[1]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  outInfo->CopyEntry(sourceInfo,
                     vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  outInfo->CopyEntry(sourceInfo,
                     vtkStreamingDemandDrivenPipeline::TIME_RANGE());

  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
               inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()),
               6);

  // Make sure that the scalar type and number of components
  // are propagated from the source not the input.
  if (vtkImageData::HasScalarType(sourceInfo))
    {
    vtkImageData::SetScalarType(vtkImageData::GetScalarType(sourceInfo),
                                outInfo);
    }
  if (vtkImageData::HasNumberOfScalarComponents(sourceInfo))
    {
    vtkImageData::SetNumberOfScalarComponents(
      vtkImageData::GetNumberOfScalarComponents(sourceInfo),
      outInfo);
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkPointInterpolator::RequestUpdateExtent(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *sourceInfo = inputVector[1]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), 0);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), 1);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), 0);
  sourceInfo->Set(
    vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(),
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()));
  sourceInfo->Set(
    vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()));
  sourceInfo->Set(
    vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS()));
  sourceInfo->Set(
    vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
    sourceInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()),
    6);

  return 1;
}

//----------------------------------------------------------------------------
void vtkPointInterpolator::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataObject *source = this->GetSource();

  this->Superclass::PrintSelf(os,indent);
  os << indent << "Source: " << source << "\n";
  os << indent << "Locator: " << this->Locator << "\n";
  os << indent << "Kernel: " << this->Kernel << "\n";
  os << indent << "Null Points Strategy: " << this->NullPointsStrategy << endl;
  os << indent << "Null Value: " << this->NullValue << "\n";
  os << indent << "Valid Points Mask Array Name: "
     << (this->ValidPointsMaskArrayName ? this->ValidPointsMaskArrayName : "(none)") << "\n";
  os << indent << "Pass Point Arrays: "
     << (this->PassPointArrays? "On" : " Off") << "\n";
  os << indent << "Pass Cell Arrays: "
     << (this->PassCellArrays? "On" : " Off") << "\n";
  os << indent << "Pass Field Arrays: "
     << (this->PassFieldArrays? "On" : " Off") << "\n";
}
