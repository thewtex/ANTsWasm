/*=========================================================================
 *
 *  Copyright NumFOCUS
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *         https://www.apache.org/licenses/LICENSE-2.0.txt
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *=========================================================================*/
#ifndef itkANTsRegistration_hxx
#define itkANTsRegistration_hxx

#include <sstream>

#include "itkCastImageFilter.h"
#include "itkResampleImageFilter.h"
#include "itkPrintHelper.h"
#include "itkANTsRegistration.h"

namespace itk
{
template <typename TFixedImage, typename TMovingImage, typename TParametersValueType>
ANTsRegistration<TFixedImage, TMovingImage, TParametersValueType>::ANTsRegistration()
{
  ProcessObject::SetNumberOfRequiredOutputs(2);
  ProcessObject::SetNumberOfRequiredInputs(2);
  ProcessObject::SetNumberOfIndexedInputs(3);
  ProcessObject::SetNumberOfIndexedOutputs(2);

  SetPrimaryInputName("FixedImage");
  AddRequiredInputName("MovingImage", 1);
  AddOptionalInputName("InitialTransform", 2);
  SetPrimaryOutputName("ForwardTransform");
  // AddRequiredOutputName("InverseTransform", 1); // this method does not exist in ProcessObject

  this->ProcessObject::SetNthOutput(0, MakeOutput(0));
  this->ProcessObject::SetNthOutput(1, MakeOutput(1));
}


template <typename TFixedImage, typename TMovingImage, typename TParametersValueType>
void
ANTsRegistration<TFixedImage, TMovingImage, TParametersValueType>::PrintSelf(std::ostream & os, Indent indent) const
{
  using namespace print_helper;
  Superclass::PrintSelf(os, indent);
  os << indent << "TypeOfTransform: " << this->m_TypeOfTransform << '\n';
  os << indent << "AffineMetric: " << this->m_AffineMetric << '\n';
  os << indent << "SynMetric: " << this->m_SynMetric << '\n';

  os << indent << "GradientStep: " << this->m_GradientStep << '\n';
  os << indent << "FlowSigma: " << this->m_FlowSigma << '\n';
  os << indent << "TotalSigma: " << this->m_TotalSigma << '\n';
  os << indent << "SamplingRate: " << this->m_SamplingRate << '\n';
  os << indent << "NumberOfBins: " << this->m_NumberOfBins << '\n';
  os << indent << "RandomSeed: " << this->m_RandomSeed << '\n';
  os << indent << "SmoothingInPhysicalUnits: " << (this->m_SmoothingInPhysicalUnits ? "On" : "Off") << '\n';
  os << indent << "UseGradientFilter: " << (this->m_UseGradientFilter ? "On" : "Off") << '\n';
  os << indent << "Radius: " << this->m_Radius << '\n';
  os << indent << "CollapseCompositeTransform: " << (this->m_CollapseCompositeTransform ? "On" : "Off") << '\n';
  os << indent << "MaskAllStages: " << (this->m_MaskAllStages ? "On" : "Off") << '\n';

  os << indent << "SynIterations: " << this->m_SynIterations << '\n';
  os << indent << "AffineIterations: " << this->m_AffineIterations << '\n';
  os << indent << "ShrinkFactors: " << this->m_ShrinkFactors << '\n';
  os << indent << "SmoothingSigmas: " << this->m_SmoothingSigmas << '\n';

  os << indent << "RestrictTransformation: " << this->m_RestrictTransformation << std::endl;

  this->m_Helper->Print(os, indent);
}


template <typename TFixedImage, typename TMovingImage, typename TParametersValueType>
void
ANTsRegistration<TFixedImage, TMovingImage, TParametersValueType>::SetFixedImage(const FixedImageType * image)
{
  if (image != this->GetFixedImage())
  {
    this->ProcessObject::SetNthInput(0, const_cast<FixedImageType *>(image));
    this->Modified();
  }
}


template <typename TFixedImage, typename TMovingImage, typename TParametersValueType>
auto
ANTsRegistration<TFixedImage, TMovingImage, TParametersValueType>::GetFixedImage() const -> const FixedImageType *
{
  return static_cast<const FixedImageType *>(this->ProcessObject::GetInput(0));
}


template <typename TFixedImage, typename TMovingImage, typename TParametersValueType>
void
ANTsRegistration<TFixedImage, TMovingImage, TParametersValueType>::SetMovingImage(const MovingImageType * image)
{
  if (image != this->GetMovingImage())
  {
    this->ProcessObject::SetNthInput(1, const_cast<MovingImageType *>(image));
    this->Modified();
  }
}


template <typename TFixedImage, typename TMovingImage, typename TParametersValueType>
auto
ANTsRegistration<TFixedImage, TMovingImage, TParametersValueType>::GetMovingImage() const -> const MovingImageType *
{
  return static_cast<const MovingImageType *>(this->ProcessObject::GetInput(1));
}

template <typename TFixedImage, typename TMovingImage, typename TParametersValueType>
auto
ANTsRegistration<TFixedImage, TMovingImage, TParametersValueType>::GetWarpedMovingImage() const ->
  typename MovingImageType::Pointer
{
  using ResampleFilterType =
    ResampleImageFilter<MovingImageType, MovingImageType, ParametersValueType, ParametersValueType>;
  typename ResampleFilterType::Pointer resampleFilter = ResampleFilterType::New();
  resampleFilter->SetInput(this->GetMovingImage());
  resampleFilter->SetTransform(this->GetForwardTransform());
  resampleFilter->SetOutputParametersFromImage(this->GetFixedImage());
  resampleFilter->Update();
  return resampleFilter->GetOutput();
}

template <typename TFixedImage, typename TMovingImage, typename TParametersValueType>
auto
ANTsRegistration<TFixedImage, TMovingImage, TParametersValueType>::GetWarpedFixedImage() const ->
  typename FixedImageType::Pointer
{
  using ResampleFilterType =
    ResampleImageFilter<FixedImageType, FixedImageType, ParametersValueType, ParametersValueType>;
  typename ResampleFilterType::Pointer resampleFilter = ResampleFilterType::New();
  resampleFilter->SetInput(this->GetFixedImage());
  resampleFilter->SetTransform(this->GetInverseTransform());
  resampleFilter->SetOutputParametersFromImage(this->GetMovingImage());
  resampleFilter->Update();
  return resampleFilter->GetOutput();
}

template <typename TFixedImage, typename TMovingImage, typename TParametersValueType>
void
ANTsRegistration<TFixedImage, TMovingImage, TParametersValueType>::SetFixedMask(const LabelImageType * mask)
{
  if (mask != this->GetFixedMask())
  {
    this->ProcessObject::SetInput("FixedMask", const_cast<LabelImageType *>(mask));
    this->Modified();
  }
}

template <typename TFixedImage, typename TMovingImage, typename TParametersValueType>
auto
ANTsRegistration<TFixedImage, TMovingImage, TParametersValueType>::GetFixedMask() const -> const LabelImageType *
{
  return static_cast<const LabelImageType *>(this->ProcessObject::GetInput("FixedMask"));
}

template <typename TFixedImage, typename TMovingImage, typename TParametersValueType>
inline void
ANTsRegistration<TFixedImage, TMovingImage, TParametersValueType>::SetMovingMask(const LabelImageType * mask)
{
  if (mask != this->GetMovingMask())
  {
    this->ProcessObject::SetInput("MovingMask", const_cast<LabelImageType *>(mask));
    this->Modified();
  }
}

template <typename TFixedImage, typename TMovingImage, typename TParametersValueType>
auto
ANTsRegistration<TFixedImage, TMovingImage, TParametersValueType>::GetMovingMask() const -> const LabelImageType *
{
  return static_cast<const LabelImageType *>(this->ProcessObject::GetInput("MovingMask"));
}


template <typename TFixedImage, typename TMovingImage, typename TParametersValueType>
auto
ANTsRegistration<TFixedImage, TMovingImage, TParametersValueType>::GetOutput(DataObjectPointerArraySizeType index)
  -> DecoratedOutputTransformType *
{
  return static_cast<DecoratedOutputTransformType *>(this->ProcessObject::GetOutput(index));
}


template <typename TFixedImage, typename TMovingImage, typename TParametersValueType>
auto
ANTsRegistration<TFixedImage, TMovingImage, TParametersValueType>::GetOutput(DataObjectPointerArraySizeType index) const
  -> const DecoratedOutputTransformType *
{
  return static_cast<const DecoratedOutputTransformType *>(this->ProcessObject::GetOutput(index));
}


template <typename TFixedImage, typename TMovingImage, typename TParametersValueType>
void
ANTsRegistration<TFixedImage, TMovingImage, TParametersValueType>::SetInput(unsigned               index,
                                                                            const FixedImageType * image)
{
  if (index == 0)
  {
    this->SetFixedImage(image);
  }
  else if (index == 1)
  {
    this->SetMovingImage(reinterpret_cast<const MovingImageType *>(image));
  }
  else
  {
    itkExceptionMacro(<< "Invalid index: " << index << ". Expected 0 (fixed) or 1 (moving).");
  }
}


template <typename TFixedImage, typename TMovingImage, typename TParametersValueType>
void
ANTsRegistration<TFixedImage, TMovingImage, TParametersValueType>::AllocateOutputs()
{
  const DecoratedOutputTransformType * decoratedOutputForwardTransform = this->GetOutput(0);
  if (!decoratedOutputForwardTransform || !decoratedOutputForwardTransform->Get())
  {
    this->ProcessObject::SetNthOutput(0, MakeOutput(0));
  }

  const DecoratedOutputTransformType * decoratedOutputInverseTransform = this->GetOutput(1);
  if (!decoratedOutputInverseTransform || !decoratedOutputInverseTransform->Get())
  {
    this->ProcessObject::SetNthOutput(1, MakeOutput(1));
  }
}


template <typename TFixedImage, typename TMovingImage, typename TParametersValueType>
auto
ANTsRegistration<TFixedImage, TMovingImage, TParametersValueType>::MakeOutput(DataObjectPointerArraySizeType)
  -> DataObjectPointer
{
  typename OutputTransformType::Pointer ptr;
  Self::MakeOutputTransform(ptr);
  typename DecoratedOutputTransformType::Pointer decoratedOutputTransform = DecoratedOutputTransformType::New();
  decoratedOutputTransform->Set(ptr);
  return decoratedOutputTransform;
}


template <typename TFixedImage, typename TMovingImage, typename TParametersValueType>
template <typename TImage>
auto
ANTsRegistration<TFixedImage, TMovingImage, TParametersValueType>::CastImageToInternalType(
  const TImage * inputImage) -> typename InternalImageType::Pointer
{
  using CastFilterType = CastImageFilter<TImage, InternalImageType>;
  typename CastFilterType::Pointer castFilter = CastFilterType::New();
  castFilter->SetInput(inputImage);
  castFilter->Update();
  typename InternalImageType::Pointer outputImage = castFilter->GetOutput();
  outputImage->DisconnectPipeline();
  return outputImage;
}


template <typename TFixedImage, typename TMovingImage, typename TParametersValueType>
void
ANTsRegistration<TFixedImage, TMovingImage, TParametersValueType>::SingleStageRegistration(
  typename RegistrationHelperType::XfrmMethod xfrmMethod,
  const InitialTransformType *                initialTransform,
  typename InternalImageType::Pointer         fixedImage,
  typename InternalImageType::Pointer         movingImage,
  bool                                        useMasks,
  unsigned                                    nTimeSteps)
{
  m_Helper = RegistrationHelperType::New(); // a convenient way to reset the helper
  std::stringstream helperLogStream;
  m_Helper->SetLogStream(helperLogStream);
  m_Helper->SetMovingInitialTransform(initialTransform);

  if (useMasks)
  {
    typename LabelImageType::Pointer fixedMask(const_cast<LabelImageType *>(this->GetFixedMask()));
    if (fixedMask != nullptr)
    {
      m_Helper->AddFixedImageMask(fixedMask);
    }
    typename LabelImageType::Pointer movingMask(const_cast<LabelImageType *>(this->GetMovingMask()));
    if (movingMask != nullptr)
    {
      m_Helper->AddMovingImageMask(movingMask);
    }
  }

  bool affineType = true;
  if (xfrmMethod != RegistrationHelperType::XfrmMethod::UnknownXfrm)
  {
    switch (xfrmMethod)
    {
      case RegistrationHelperType::Affine: {
        m_Helper->AddAffineTransform(m_GradientStep);
      }
      break;
      case RegistrationHelperType::Rigid: {
        m_Helper->AddRigidTransform(m_GradientStep);
      }
      break;
      case RegistrationHelperType::CompositeAffine: {
        m_Helper->AddCompositeAffineTransform(m_GradientStep);
      }
      break;
      case RegistrationHelperType::Similarity: {
        m_Helper->AddSimilarityTransform(m_GradientStep);
      }
      break;
      case RegistrationHelperType::Translation: {
        m_Helper->AddTranslationTransform(m_GradientStep);
      }
      break;
      case RegistrationHelperType::GaussianDisplacementField: {
        m_Helper->AddGaussianDisplacementFieldTransform(m_GradientStep, m_FlowSigma, m_TotalSigma);
        affineType = false;
      }
      break;
      case RegistrationHelperType::SyN: {
        m_Helper->AddSyNTransform(m_GradientStep, m_FlowSigma, m_TotalSigma);
        affineType = false;
      }
      break;
      case RegistrationHelperType::TimeVaryingVelocityField: {
        m_Helper->AddTimeVaryingVelocityFieldTransform(m_GradientStep, nTimeSteps, m_FlowSigma, 0.0, m_TotalSigma, 0.0);
        affineType = false;
      }
      break;
      // BSpline is not available in ANTsPy, but is easy to support here
      case RegistrationHelperType::BSpline: {
        auto meshSizeAtBaseLevel =
          m_Helper->CalculateMeshSizeForSpecifiedKnotSpacing(fixedImage, 50, 3); // TODO: expose grid spacing?
        m_Helper->AddBSplineTransform(m_GradientStep, meshSizeAtBaseLevel);
        affineType = false;
      }
      break;
      // BSplineSyN is not available in ANTsPy, but is used in faces example
      case RegistrationHelperType::BSplineSyN: {
        unsigned int splineOrder = 3;
        std::vector<unsigned int> meshSizeForTheUpdateField =
          m_Helper->CalculateMeshSizeForSpecifiedKnotSpacing(fixedImage, 75, splineOrder); // TODO: expose grid spacing?
        std::vector<unsigned int> meshSizeForTheTotalField(ImageDimension, 0);
        m_Helper->AddBSplineSyNTransform(
          m_GradientStep, meshSizeForTheUpdateField, meshSizeForTheTotalField, splineOrder);
        affineType = false;
      }
      break;
      // These are not available in ANTsPy, so we don't support them either
      case RegistrationHelperType::BSplineDisplacementField:
      case RegistrationHelperType::TimeVaryingBSplineVelocityField:
      case RegistrationHelperType::Exponential:
      case RegistrationHelperType::BSplineExponential:
        itkExceptionMacro(<< "Unsupported transform type: " << this->GetTypeOfTransform());
      default:
        itkExceptionMacro(<< "Transform known to ANTs helper, but not to us: " << this->GetTypeOfTransform());
    }
  }

  std::vector<unsigned int> iterations;
  if (affineType)
  {
    iterations = m_AffineIterations;
  }
  else
  {
    iterations = m_SynIterations;
  }

  // set the vector-vector parameters
  m_Helper->SetIterations({ iterations });
  if (iterations.size() == m_SmoothingSigmas.size())
  {
    m_Helper->SetSmoothingSigmas({ m_SmoothingSigmas });
  }
  else
  {
    int sizeDiff = m_SmoothingSigmas.size() - iterations.size();
    if (sizeDiff < 0)
    {
      using namespace print_helper;
      itkExceptionMacro(<< "SmoothingSigmas vector: " << m_SmoothingSigmas
                        << " is shorter than iterations: " << iterations);
    }
    m_Helper->SetSmoothingSigmas({ { m_SmoothingSigmas.begin() + sizeDiff, m_SmoothingSigmas.end() } });
  }
  if (iterations.size() == m_ShrinkFactors.size())
  {
    m_Helper->SetShrinkFactors({ m_ShrinkFactors });
  }
  else
  {
    int sizeDiff = m_ShrinkFactors.size() - iterations.size();
    if (sizeDiff < 0)
    {
      using namespace print_helper;
      itkExceptionMacro(<< "ShrinkFactors vector: " << m_ShrinkFactors
                        << " is shorter than iterations: " << iterations);
    }
    m_Helper->SetShrinkFactors({ { m_ShrinkFactors.begin() + sizeDiff, m_ShrinkFactors.end() } });
  }

  m_Helper->SetSmoothingSigmasAreInPhysicalUnits({ m_SmoothingInPhysicalUnits });
  if (m_RandomSeed != 0)
  {
    m_Helper->SetRegistrationRandomSeed(m_RandomSeed);
  }
  if (!m_RestrictTransformation.empty())
  {
    m_Helper->SetRestrictDeformationOptimizerWeights({ m_RestrictTransformation });
  }

  // match the length of the iterations vector by these defaulted parameters
  std::vector<unsigned int> windows(iterations.size(), 10);
  m_Helper->SetConvergenceWindowSizes({ windows });
  std::vector<ParametersValueType> thresholds(iterations.size(), 1e-6);
  m_Helper->SetConvergenceThresholds({ thresholds });

  std::string metricType;
  if (affineType)
  {
    metricType = this->GetAffineMetric();
  }
  else
  {
    metricType = this->GetSynMetric();
  }
  std::transform(metricType.begin(), metricType.end(), metricType.begin(), tolower);
  if (metricType == "jhmi")
  {
    metricType = "mi2"; // ANTs uses "mi" for Mattes MI, see:
    // https://github.com/ANTsX/ANTs/blob/v2.5.1/Examples/itkantsRegistrationHelper.hxx#L145-L152
  }
  typename RegistrationHelperType::MetricEnumeration currentMetric = m_Helper->StringToMetricType(metricType);

  m_Helper->AddMetric(currentMetric,
                      fixedImage,
                      movingImage,
                      nullptr,
                      nullptr,
                      nullptr,
                      nullptr,
                      0u,
                      1.0,
                      RegistrationHelperType::regular,
                      m_NumberOfBins,
                      m_Radius,
                      m_UseGradientFilter,
                      false,
                      1.0,
                      50u,
                      1.1,
                      false,
                      m_SamplingRate,
                      std::sqrt(5),
                      std::sqrt(5));
  int retVal = m_Helper->DoRegistration();
  if (retVal != EXIT_SUCCESS)
  {
    itkExceptionMacro(<< "Registration failed. Helper's accumulated output:\n " << helperLogStream.str());
  }
  else
  {
    itkDebugMacro("Registration successful. Helper's accumulated output:\n " << helperLogStream.str());
  }
}


template <typename TFixedImage, typename TMovingImage, typename TParametersValueType>
void
ANTsRegistration<TFixedImage, TMovingImage, TParametersValueType>::GenerateData()
{
  this->AllocateOutputs();

  this->UpdateProgress(0.01);

  typename CompositeTransformType::Pointer emptyComposite = CompositeTransformType::New();
  const InitialTransformType *             initialTransform = emptyComposite;
  const DecoratedInitialTransformType *    decoratedInitialTransform = this->GetInitialTransformInput();
  if (decoratedInitialTransform != nullptr)
  {
    initialTransform = decoratedInitialTransform->Get();
  }

  typename InternalImageType::Pointer fixedImage = this->CastImageToInternalType(this->GetFixedImage());
  typename InternalImageType::Pointer movingImage = this->CastImageToInternalType(this->GetMovingImage());

  std::string whichTransform = this->GetTypeOfTransform();
  std::transform(whichTransform.begin(), whichTransform.end(), whichTransform.begin(), tolower);
  typename RegistrationHelperType::XfrmMethod xfrmMethod = m_Helper->StringToXfrmMethod(whichTransform);

  if (whichTransform == "synonly")
  {
    SingleStageRegistration(RegistrationHelperType::XfrmMethod::SyN, initialTransform, fixedImage, movingImage, true);
  }
  else if (whichTransform == "syn") // this is Affine + deformable
  {
    SingleStageRegistration(
      RegistrationHelperType::XfrmMethod::Affine, initialTransform, fixedImage, movingImage, m_MaskAllStages);
    this->UpdateProgress(0.15);
    typename OutputTransformType::Pointer intermediateTransform = m_Helper->GetModifiableCompositeTransform();
    SingleStageRegistration(
      RegistrationHelperType::XfrmMethod::SyN, intermediateTransform, fixedImage, movingImage, true);
  }
  else if (xfrmMethod != RegistrationHelperType::XfrmMethod::UnknownXfrm) // a plain single-stage transform
  {
    SingleStageRegistration(xfrmMethod, initialTransform, fixedImage, movingImage, true);
  }
  else if (whichTransform == "quickrigid")
  {
    auto originalIterations = m_AffineIterations;
    m_AffineIterations = { 20, 20, 0, 0 };
    SingleStageRegistration(RegistrationHelperType::XfrmMethod::Rigid, initialTransform, fixedImage, movingImage, true);
    m_AffineIterations = originalIterations;
  }
  else if (whichTransform == "trsaa")
  {
    auto originalGradientStep = m_GradientStep;
    m_GradientStep = 1.0;
    SingleStageRegistration(
      RegistrationHelperType::XfrmMethod::Translation, initialTransform, fixedImage, movingImage, m_MaskAllStages);
    this->UpdateProgress(0.15);
    typename OutputTransformType::Pointer intermediateTransform = m_Helper->GetModifiableCompositeTransform();
    SingleStageRegistration(
      RegistrationHelperType::XfrmMethod::Rigid, intermediateTransform, fixedImage, movingImage, m_MaskAllStages);
    this->UpdateProgress(0.30);
    intermediateTransform = m_Helper->GetModifiableCompositeTransform();
    SingleStageRegistration(
      RegistrationHelperType::XfrmMethod::Similarity, intermediateTransform, fixedImage, movingImage, m_MaskAllStages);
    this->UpdateProgress(0.45);
    intermediateTransform = m_Helper->GetModifiableCompositeTransform();
    SingleStageRegistration(
      RegistrationHelperType::XfrmMethod::Affine, intermediateTransform, fixedImage, movingImage, m_MaskAllStages);
    this->UpdateProgress(0.65);
    intermediateTransform = m_Helper->GetModifiableCompositeTransform();
    SingleStageRegistration(
      RegistrationHelperType::XfrmMethod::Affine, intermediateTransform, fixedImage, movingImage, true);
    m_GradientStep = originalGradientStep;
  }
  else if (whichTransform == "elastic")
  {
    SingleStageRegistration(
      RegistrationHelperType::XfrmMethod::Affine, initialTransform, fixedImage, movingImage, m_MaskAllStages);
    this->UpdateProgress(0.15);
    typename OutputTransformType::Pointer intermediateTransform = m_Helper->GetModifiableCompositeTransform();
    SingleStageRegistration(RegistrationHelperType::XfrmMethod::GaussianDisplacementField,
                            intermediateTransform,
                            fixedImage,
                            movingImage,
                            true);
  }
  else if (whichTransform == "synra")
  {
    SingleStageRegistration(
      RegistrationHelperType::XfrmMethod::Rigid, initialTransform, fixedImage, movingImage, m_MaskAllStages);
    this->UpdateProgress(0.15);
    typename OutputTransformType::Pointer intermediateTransform = m_Helper->GetModifiableCompositeTransform();
    SingleStageRegistration(
      RegistrationHelperType::XfrmMethod::Affine, intermediateTransform, fixedImage, movingImage, m_MaskAllStages);
    this->UpdateProgress(0.30);
    intermediateTransform = m_Helper->GetModifiableCompositeTransform();
    SingleStageRegistration(
      RegistrationHelperType::XfrmMethod::SyN, intermediateTransform, fixedImage, movingImage, true);
  }
  else if (whichTransform == "syncc")
  {
    std::string originalMetric = m_AffineMetric;
    m_AffineMetric = "CC";
    SingleStageRegistration(
      RegistrationHelperType::XfrmMethod::Affine, initialTransform, fixedImage, movingImage, m_MaskAllStages);
    m_AffineMetric = originalMetric;
    this->UpdateProgress(0.15);
    originalMetric = m_SynMetric;
    m_SynMetric = "CC";
    typename OutputTransformType::Pointer intermediateTransform = m_Helper->GetModifiableCompositeTransform();
    SingleStageRegistration(
      RegistrationHelperType::XfrmMethod::SyN, intermediateTransform, fixedImage, movingImage, true);
    m_SynMetric = originalMetric;
  }
  else if (whichTransform.substr(0, 3) == "tv[") // TV[n]
  {
    unsigned tsl = whichTransform.size(); // transform string length
    if (tsl < 5 || whichTransform[tsl - 1] != ']')
    {
      itkExceptionMacro(<< "Invalid transform type: " << whichTransform
                        << "\nExpected format: TV[n]\nwhere n is number of time points");
    }
    unsigned timePoints = 4;
    try
    {
      timePoints = std::stoul(whichTransform.substr(3, tsl - 4));
    }
    catch (const std::exception & err)
    {
      itkExceptionMacro(<< "Cannot interpret: '" << whichTransform.substr(3, tsl - 4)
                        << "' as a number. Inner exception: " << err.what());
    }
    SingleStageRegistration(RegistrationHelperType::XfrmMethod::TimeVaryingVelocityField,
                            initialTransform,
                            fixedImage,
                            movingImage,
                            true,
                            timePoints);
  }
  else
  {
    itkExceptionMacro(<< "Unsupported transform type: " << this->GetTypeOfTransform());
  }
  this->UpdateProgress(0.95);

  typename OutputTransformType::Pointer forwardTransform = m_Helper->GetModifiableCompositeTransform();
  if (m_CollapseCompositeTransform)
  {
    forwardTransform = m_Helper->CollapseCompositeTransform(forwardTransform);
  }
  this->SetForwardTransform(forwardTransform);

  typename OutputTransformType::Pointer inverseTransform = OutputTransformType::New();
  if (forwardTransform->GetInverse(inverseTransform))
  {
    this->SetInverseTransform(inverseTransform);
  }
  else
  {
    this->SetInverseTransform(nullptr);
  }

  this->UpdateProgress(1.0);
}

} // end namespace itk

#endif // itkANTsRegistration_hxx
