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
#ifndef itkANTsGroupwiseRegistration_hxx
#define itkANTsGroupwiseRegistration_hxx

#include <sstream>

#include "itkPrintHelper.h"
#include "itkANTsGroupwiseRegistration.h" // needed by Visual Studio for code completion
#include "itkImageDuplicator.h"
#include "itkWeightedAddImageFilter.h"
#include "itkAverageAffineTransformFunction.h"
#include "itkLaplacianSharpeningImageFilter.h"

namespace // anonymous namespace for debug functions
{
template <typename TransformType>
void
WriteTransform(const TransformType * transform, std::string filename)
{
  constexpr unsigned Dimension = TransformType::OutputSpaceDimension;
  using Affine3D = itk::AffineTransform<typename TransformType::ParametersValueType, 3>;
  using Affine2D = itk::AffineTransform<typename TransformType::ParametersValueType, 2>;
  using TransformWriterType = itk::TransformFileWriterTemplate<typename TransformType::ParametersValueType>;
  typename TransformWriterType::Pointer tWriter = TransformWriterType::New();
  tWriter->SetFileName(filename);

  if (Dimension == 2 && std::string(transform->GetNameOfClass()) == "AffineTransform")
  {
    typename Affine2D::ConstPointer t2d = dynamic_cast<const Affine2D *>(transform);
    // convert into affine which Slicer can read
    typename Affine3D::MatrixType      m;
    typename Affine3D::TranslationType t;
    t.Fill(0);
    typename Affine3D::InputPointType c;
    c.Fill(0);
    for (unsigned int i = 0; i < Dimension; i++)
    {
      for (unsigned int j = 0; j < Dimension; j++)
      {
        m[i][j] = t2d->GetMatrix()(i, j);
      }
      t[i] = t2d->GetMatrix()(i, Dimension);
      c[i] = t2d->GetCenter()[i];
    }
    m(Dimension, Dimension) = 1.0;

    typename Affine3D::Pointer aTr = Affine3D::New();
    aTr->SetCenter(c);
    aTr->SetMatrix(m);
    aTr->SetOffset(t);
    tWriter->SetInput(aTr);
  }
  else
  {
    tWriter->SetInput(transform);
  }
  tWriter->Update();
}
} // namespace

namespace itk
{

template <typename TImage, typename TTemplateImage, typename TParametersValueType>
ANTsGroupwiseRegistration<TImage, TTemplateImage, TParametersValueType>::ANTsGroupwiseRegistration()
{
  this->SetPrimaryInputName("InitialTemplate");
  this->SetPrimaryOutputName("OptimizedImage");

  this->SetInput(0, TemplateImageType::New()); // empty initial template

  this->GetMultiThreader()->SetMaximumNumberOfThreads(1); // registrations are already multi-threaded
}


template <typename TImage, typename TTemplateImage, typename TParametersValueType>
void
ANTsGroupwiseRegistration<TImage, TTemplateImage, TParametersValueType>::PrintSelf(std::ostream & os,
                                                                                   Indent         indent) const
{
  using namespace print_helper;
  Superclass::PrintSelf(os, indent);

  os << indent << "GradientStep: " << this->m_GradientStep << std::endl;
  os << indent << "BlendingWeight: " << this->m_BlendingWeight << std::endl;
  os << indent << "UseNoRigid: " << (this->m_UseNoRigid ? "On" : "Off") << std::endl;
  os << indent << "Iterations: " << this->m_Iterations << std::endl;
  os << indent << "Weights: " << this->m_Weights << std::endl;

  os << indent << "ImageList: " << std::endl;
  // unsigned i = 0;
  // for (const auto & image : this->m_ImageList)
  // {
  //   os << indent.GetNextIndent() << "Image" << i++ << ": ";
  //   image->Print(os, indent.GetNextIndent());
  // }

  os << indent << "PairwiseRegistration: ";
  if (this->m_PairwiseRegistration)
  {
    this->m_PairwiseRegistration->Print(os, indent.GetNextIndent());
  }
  else
  {
    os << "nullptr" << std::endl;
  }
}


template <typename TImage, typename TTemplateImage, typename TParametersValueType>
void
ANTsGroupwiseRegistration<TImage, TTemplateImage, TParametersValueType>::VerifyInputInformation() const
{
  if (m_ImageList.empty())
  {
    itkExceptionMacro("No input images provided.");
  }

  if (m_Weights.size() > 0 && m_Weights.size() != m_ImageList.size())
  {
    itkExceptionMacro("The number of weights is different from the number of images.");
  }

  if (m_ImageList.size() < 2)
  {
    itkExceptionMacro("At least two input images are required.");
  }
}


template <typename TImage, typename TTemplateImage, typename TParametersValueType>
DataObject::Pointer
ANTsGroupwiseRegistration<TImage, TTemplateImage, TParametersValueType>::MakeOutput(DataObjectPointerArraySizeType)
{
  typename OutputTransformType::Pointer ptr;
  Self::MakeOutputTransform(ptr);
  typename DecoratedOutputTransformType::Pointer decoratedOutputTransform = DecoratedOutputTransformType::New();
  decoratedOutputTransform->Set(ptr);
  return decoratedOutputTransform;
}


template <typename TImage, typename TTemplateImage, typename TParametersValueType>
auto
ANTsGroupwiseRegistration<TImage, TTemplateImage, TParametersValueType>::DuplicateImage(const TemplateImageType * image)
  -> typename TemplateImageType::Pointer
{
  using DuplicatorType = ImageDuplicator<TemplateImageType>;
  typename DuplicatorType::Pointer duplicator = DuplicatorType::New();
  duplicator->SetInputImage(image);
  duplicator->Update();
  return duplicator->GetOutput();
}


template <typename TImage, typename TTemplateImage, typename TParametersValueType>
template <typename TOutputImage, typename TInputImage>
auto
itk::ANTsGroupwiseRegistration<TImage, TTemplateImage, TParametersValueType>::ResampleToTarget(
  const TInputImage *                  input,
  const TemplateImageType *            target,
  typename TransformType::ConstPointer transform) -> typename TOutputImage::Pointer
{
  using ResampleFilterType = ResampleImageFilter<TInputImage, TOutputImage, ParametersValueType, ParametersValueType>;
  typename ResampleFilterType::Pointer resampleFilter = ResampleFilterType::New();
  resampleFilter->SetInput(input);
  if (transform)
  {
    resampleFilter->SetTransform(transform);
  }
  resampleFilter->SetOutputParametersFromImage(target);
  resampleFilter->Update();
  return resampleFilter->GetOutput();
}


template <typename TImage, typename TTemplateImage, typename TParametersValueType>
auto
ANTsGroupwiseRegistration<TImage, TTemplateImage, TParametersValueType>::AverageTransformedImages(
  const std::vector<typename AffineType::ConstPointer> & affineList) -> typename TemplateImageType::Pointer
{
  assert(m_ImageList.size() == affineList.size());

  typename TemplateImageType::Pointer average = TemplateImageType::New();
  average->CopyInformation(m_ImageList[0]);
  average->SetRegions(m_ImageList[0]->GetLargestPossibleRegion());
  average->Allocate(true); // initialize to zero

  for (unsigned i = 0; i < m_ImageList.size(); ++i)
  {
    typename TemplateImageType::Pointer resampledImage =
      ResampleToTarget<TemplateImageType, ImageType>(m_ImageList[i], average, affineList[i]);

    using AddImageFilterType = WeightedAddImageFilter<TemplateImageType, TemplateImageType, TemplateImageType>;
    typename AddImageFilterType::Pointer addImageFilter = AddImageFilterType::New();
    addImageFilter->SetInPlace(true);
    addImageFilter->SetInput1(average);
    addImageFilter->SetInput2(resampledImage);
    addImageFilter->SetAlpha(1.0 - m_Weights[i]);
    addImageFilter->Update();
    average = addImageFilter->GetOutput();
  }

  return average;
}

template <typename TImage, typename TTemplateImage, typename TParametersValueType>
auto
ANTsGroupwiseRegistration<TImage, TTemplateImage, TParametersValueType>::AverageDisplacementFields(
  const std::vector<typename DisplacementImageType::Pointer> & dfList) -> typename DisplacementImageType::Pointer
{
  assert(dfList.size() == m_Weights.size());

  typename DisplacementImageType::Pointer average = DisplacementImageType::New();
  average->CopyInformation(dfList[0]);
  average->SetRegions(dfList[0]->GetLargestPossibleRegion());
  average->Allocate(true); // initialize to zero

  for (unsigned i = 0; i < dfList.size(); ++i)
  {
    assert(average->IsSameImageGeometryAs(dfList[i]));

    auto weightedVectorAdd = [this, i](const typename DisplacementImageType::PixelType & a,
                                       const typename DisplacementImageType::PixelType & b) {
      typename DisplacementImageType::PixelType result{ a };
      for (unsigned j = 0; j < DisplacementImageType::PixelType::Dimension; ++j)
      {
        result[j] = (1.0 - this->m_Weights[i]) * a[j] + this->m_Weights[i] * b[j];
      }
      return result;
    };

    using AddImageFilterType =
      BinaryGeneratorImageFilter<DisplacementImageType, DisplacementImageType, DisplacementImageType>;
    typename AddImageFilterType::Pointer addImageFilter = AddImageFilterType::New();
    addImageFilter->SetInPlace(true);
    addImageFilter->SetFunctor(weightedVectorAdd);
    addImageFilter->SetInput1(average);
    addImageFilter->SetInput2(dfList[i]);
    addImageFilter->Update();
    average = addImageFilter->GetOutput();
  }

  return average;
}


template <typename TImage, typename TTemplateImage, typename TParametersValueType>
void
ANTsGroupwiseRegistration<TImage, TTemplateImage, TParametersValueType>::GenerateData()
{
  this->UpdateProgress(0.0);

  if (!m_PairwiseRegistration) // TODO: allow setting a custom pairwise registration
  {
    m_PairwiseRegistration = PairwiseType::New();
    m_PairwiseRegistration->SetTypeOfTransform("SyN");
    m_PairwiseRegistration->SetTypeOfTransform("QuickRigid"); // debug
    // m_PairwiseRegistration->SetTypeOfTransform("Affine"); // debug
    m_PairwiseRegistration->DebugOn();
  }

  if (m_Weights.empty())
  {
    m_Weights.resize(m_ImageList.size(), 1.0 / m_ImageList.size());
  }
  else // normalize to sum to 1
  {
    m_Weights.resize(m_ImageList.size(), 1.0);
    TParametersValueType sum = 0;
    for (const auto & weight : m_Weights)
    {
      sum += weight;
    }
    for (auto & weight : m_Weights)
    {
      weight /= sum;
    }
  }
  m_TransformList.resize(m_ImageList.size(), nullptr);

  typename TemplateImageType::Pointer initialTemplate = dynamic_cast<TemplateImageType *>(this->GetInput(0));
  if (!initialTemplate)
  {
    itkExceptionMacro("Initial template must be a float-pixel image.");
  }

  std::vector<typename AffineType::ConstPointer> emptyAffines(m_ImageList.size(), nullptr);
  if (initialTemplate->GetLargestPossibleRegion().GetNumberOfPixels() > 0)
  {
    // copy the initial template into the output (the current average template)
    this->GraftOutput(this->DuplicateImage(initialTemplate));
  }
  else
  {
    // create a simple average of all the input images
    this->GraftOutput(this->AverageTransformedImages(emptyAffines));
  }
  this->UpdateProgress(0.01);
  WriteImage(this->GetOutput(), "initialTemplate.nrrd"); // debug

  float progressStep = 0.98 / (m_Iterations * m_ImageList.size());

  for (unsigned i = 0; i < m_Iterations; ++i)
  {
    typename TemplateImageType::Pointer xavg = this->DuplicateImage(this->GetOutput());
    WriteImage(xavg, "xavg" + std::to_string(i) + ".nrrd"); // debug
    std::vector<typename AffineType::ConstPointer>       affineList(m_ImageList.size(), nullptr);
    std::vector<typename DisplacementImageType::Pointer> dfList(m_ImageList.size(), nullptr);
    for (unsigned k = 0; k < m_ImageList.size(); ++k)
    {
      m_PairwiseRegistration->SetFixedImage(xavg);
      m_PairwiseRegistration->SetMovingImage(m_ImageList[k]);
      m_PairwiseRegistration->Update();

      const CompositeTransformType * compositeTransform = m_PairwiseRegistration->GetForwardTransform();

      affineList[k] = dynamic_cast<const AffineType *>(compositeTransform->GetFrontTransform());
      auto dfTransform = dynamic_cast<const DisplacementTransformType *>(compositeTransform->GetBackTransform());
      auto dfNonConstTransform = const_cast<DisplacementTransformType *>(dfTransform);
      if (dfNonConstTransform)
      {
        dfList[k] = dfNonConstTransform->GetDisplacementField();
      }

      if (m_KeepTransforms || (compositeTransform->GetNumberOfTransforms() == 1 && affineList[k] != nullptr))
      {
        // if the composite transform is just an affine, keep it regardless of the setting
        m_TransformList[k] = const_cast<CompositeTransformType *>(compositeTransform);
      }
      WriteTransform(affineList[k].GetPointer(), "ta" + std::to_string(i) + "_" + std::to_string(k) + ".tfm"); // debug
      WriteTransform(compositeTransform, "tc" + std::to_string(i) + "_" + std::to_string(k) + ".tfm");         // debug
      this->UpdateProgress(0.01f + progressStep * (i * m_ImageList.size() + (k + 1)));
    }

    // average transformed images, start with an all-zero image
    typename TemplateImageType::Pointer xavgNew = this->AverageTransformedImages(affineList);
    WriteImage(xavgNew, "xavgNew" + std::to_string(i) + ".nrrd"); // debug

    typename AffineType::Pointer avgAffine = AffineType::New();
    typename AffineType::Pointer avgAffineInverse = AffineType::New();

    using WarperType = AverageAffineTransformFunction<AffineType>;
    WarperType average_func;
    average_func.verbose = false;
    average_func.useRigid = !m_UseNoRigid;
    for (unsigned k = 0; k < m_ImageList.size(); ++k)
    {
      average_func.PushBackAffineTransform(affineList[k], m_Weights[k]);
    }
    average_func.AverageMultipleAffineTransform(affineList[0]->GetCenter(), avgAffine);

    bool inverseExists = avgAffine->GetInverse(avgAffineInverse);
    assert(inverseExists);

    if (std::count(dfList.begin(), dfList.end(), nullptr) == 0) // we have all the displacement fields
    {
      // average the deformation fields
      typename DisplacementImageType::Pointer wavg = this->AverageDisplacementFields(dfList);
      // WriteImage(wavg, "wavg" + std::to_string(i) + ".nrrd"); // debug

      // wavg *= -gradient_step
      typename DisplacementImageType::PixelType wscl;
      wscl.Fill(-1.0 * this->GetGradientStep());
      using MulType = MultiplyImageFilter<DisplacementImageType, DisplacementImageType, DisplacementImageType>;
      typename MulType::Pointer mul = MulType::New();
      mul->SetInPlace(true);
      mul->SetInput1(wavg);
      mul->SetConstant2(wscl);
      mul->Update();
      wavg = mul->GetOutput();

      typename DisplacementImageType::Pointer wavgA =
        ResampleToTarget<DisplacementImageType, DisplacementImageType>(wavg, xavgNew, avgAffineInverse);
      // WriteImage(wavgA, "wavgA" + std::to_string(i) + ".nrrd"); // debug

      typename DisplacementTransformType::Pointer wavgTransform = DisplacementTransformType::New();
      wavgTransform->SetDisplacementField(wavgA);

      typename CompositeTransformType::Pointer combinedTransform = CompositeTransformType::New();
      combinedTransform->AddTransform(wavgTransform);
      combinedTransform->AddTransform(avgAffineInverse);
      xavg = ResampleToTarget<TemplateImageType, TemplateImageType>(xavgNew, xavgNew, combinedTransform);
    }
    else // we only have the affine transforms
    {
      xavg = ResampleToTarget<TemplateImageType, TemplateImageType>(xavgNew, xavgNew, avgAffineInverse);
    }
    // WriteImage(xavg, "xavg" + std::to_string(i) + ".nrrd"); // debug

    if (m_BlendingWeight > 0)
    {
      using SharpenFilterType = LaplacianSharpeningImageFilter<TemplateImageType, TemplateImageType>;
      typename SharpenFilterType::Pointer sharpenFilter = SharpenFilterType::New();
      sharpenFilter->SetInput(xavg);
      sharpenFilter->Update();
      typename TemplateImageType::Pointer sharpened = sharpenFilter->GetOutput();

      using AddImageFilterType = WeightedAddImageFilter<TemplateImageType, TemplateImageType, TemplateImageType>;
      typename AddImageFilterType::Pointer addImageFilter = AddImageFilterType::New();
      addImageFilter->SetInPlace(true);
      addImageFilter->SetInput1(xavg);
      addImageFilter->SetInput2(sharpened);
      addImageFilter->SetAlpha(m_BlendingWeight);
      addImageFilter->Update();
      xavg = addImageFilter->GetOutput();
    }

    WriteImage(xavg, "avgTemplate" + std::to_string(i) + ".nrrd"); // debug
    this->GraftOutput(xavg);
  }

  this->UpdateProgress(1.0);
}

} // end namespace itk

#endif // itkANTsGroupwiseRegistration_hxx