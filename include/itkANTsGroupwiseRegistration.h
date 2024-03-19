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
#ifndef itkANTsGroupwiseRegistration_h
#define itkANTsGroupwiseRegistration_h

#include "itkImageToImageFilter.h"
#include "itkANTsRegistration.h"
#include "itkImage.h"
#include "itkDisplacementFieldTransform.h"

namespace itk
{

/** \class ANTsGroupwiseRegistration
 *
 * \brief Group-wise image registration method parameterized according to ANTsPy.
 *
 * Inputs are images to be registered, and optionally initial template.
 * Outputs are the computed template image, and forward and inverse transforms
 * for each of the input images when registered to the template.
 *
 * This is similar to ANTsPy build_template function:
 * https://github.com/ANTsX/ANTsPy/blob/master/ants/registration/build_template.py
 *
 * \ingroup ANTsWasm
 * \ingroup Registration
 *
 */
template <typename TImage,
          typename TTemplateImage = ::itk::Image<float, TImage::ImageDimension>,
          typename TParametersValueType = double>
class ANTsGroupwiseRegistration : public ImageToImageFilter<TTemplateImage, TTemplateImage>
{
public:
  ITK_DISALLOW_COPY_AND_MOVE(ANTsGroupwiseRegistration);

  static constexpr unsigned int ImageDimension = TImage::ImageDimension;

  using ImageType = TImage;
  using PixelType = typename ImageType::PixelType;
  using TemplateImageType = TTemplateImage;

  using ParametersValueType = TParametersValueType;
  using TransformType = Transform<TParametersValueType, ImageDimension, ImageDimension>;
  using CompositeTransformType = CompositeTransform<ParametersValueType, ImageDimension>;
  using OutputTransformType = CompositeTransformType;
  using DecoratedOutputTransformType = DataObjectDecorator<OutputTransformType>;

  /** Standard class aliases. */
  using Self = ANTsGroupwiseRegistration<ImageType, TTemplateImage, ParametersValueType>;
  using Superclass = ImageToImageFilter<TTemplateImage, TTemplateImage>;
  using Pointer = SmartPointer<Self>;
  using ConstPointer = SmartPointer<const Self>;

  /** Run-time type information. */
  itkTypeMacro(ANTsGroupwiseRegistration, ImageToImageFilter);

  /** Standard New macro. */
  itkNewMacro(Self);


  /** Set/Get the initial template image. */
  void
  SetInitialTemplateImage(const TemplateImageType * initialTemplate)
  {
    this->SetNthInput(0, const_cast<TemplateImageType *>(initialTemplate)); // the primary input
  }
  const TemplateImageType *
  GetInitialTemplateImage()
  {
    return static_cast<TemplateImageType *>(this->GetInput(0)); // the primary input
  }

  /** Get the optimal template image. */
  TemplateImageType *
  GetTemplateImage()
  {
    return this->GetOutput(0); // this is just the primary output
  }

  /** Set/Get step size for shape update gradient. */
  itkSetMacro(GradientStep, ParametersValueType);
  itkGetMacro(GradientStep, ParametersValueType);

  /** Set/Get the weight for image blending. Zero disables it. */
  itkSetMacro(BlendingWeight, ParametersValueType);
  itkGetMacro(BlendingWeight, ParametersValueType);

  /** Set/Get whether template update step uses the rigid component. */
  itkSetMacro(UseNoRigid, bool);
  itkGetMacro(UseNoRigid, bool);
  itkBooleanMacro(UseNoRigid);

  /** Set/Get number of template building iterations. */
  itkSetMacro(Iterations, unsigned int);
  itkGetMacro(Iterations, unsigned int);


  /** Set/Get whether we should keep registration transforms in memory.
   * If true, transforms which register each of the input images to the template
   * will be available via GetTransform(index) after the registration finishes.
   * Off by default, to conserve memory. */
  itkSetMacro(KeepTransforms, bool);
  itkGetMacro(KeepTransforms, bool);
  itkBooleanMacro(KeepTransforms);

  /** Set/Get the weight for each image. */
  itkSetMacro(Weights, std::vector<ParametersValueType>);
  itkGetConstReferenceMacro(Weights, std::vector<ParametersValueType>);

  /** Set the images to register. */
  itkSetMacro(ImageList, std::vector<typename ImageType::Pointer>);

  /** Add an image to the list of images to register. */
  virtual void
  AddImage(const ImageType * image)
  {
    m_ImageList.push_back(const_cast<ImageType *>(image));
  }

  /** Clear the list of images to register. */
  virtual void
  ClearImageList()
  {
    m_ImageList.clear();
  }

  /** Returns the transform which registers the image with the provided index to the average template.
   * To make sure this is available, KeepTransformsOn() should be set be called. */
  const OutputTransformType *
  GetTransform(unsigned imageIndex) const
  {
    return m_TransformList[imageIndex];
  }

  using PairwiseType = ANTsRegistration<TemplateImageType, ImageType, ParametersValueType>;
  /** Set/Get step size for shape update gradient. */
  itkSetObjectMacro(PairwiseRegistration, PairwiseType);
  itkGetModifiableObjectMacro(PairwiseRegistration, PairwiseType);

  using ProcessObject::AddInput;
  using ProcessObject::RemoveInput;
  using ProcessObject::GetInput;

  /** This filter only deals with whole images. */
  void
  GenerateInputRequestedRegion() override
  {
    auto * inputPtr = dynamic_cast<TTemplateImage *>(this->GetInput(0));
    inputPtr->SetRequestedRegionToLargestPossibleRegion();
  }

  /** Request the largest possible region on all outputs. */
  void
  EnlargeOutputRequestedRegion(DataObject * output) override
  {
    output->SetRequestedRegionToLargestPossibleRegion();
  }

  void
  GenerateOutputInformation() override;

protected:
  ANTsGroupwiseRegistration();
  ~ANTsGroupwiseRegistration() override = default;

  /** Make a DataObject of the correct type to be used as the specified output. */
  using DataObjectPointerArraySizeType = ProcessObject::DataObjectPointerArraySizeType;
  DataObject::Pointer MakeOutput(DataObjectPointerArraySizeType) override;

  void
  PrintSelf(std::ostream & os, Indent indent) const override;

  void
  GenerateData() override;

  void
  VerifyInputInformation() const override;

  // helper function to create the right kind of concrete transform
  template <typename TTransform>
  static void
  MakeOutputTransform(SmartPointer<TTransform> & ptr)
  {
    ptr = TTransform::New();
  }

  typename TemplateImageType::Pointer
  DuplicateImage(const TemplateImageType * image);

  template <typename TOutputImage, typename TInputImage>
  typename TOutputImage::Pointer
  ResampleToTarget(const TInputImage *                  input,
                   const TemplateImageType *            target,
                   typename TransformType::ConstPointer transform);

  ParametersValueType m_GradientStep{ 0.2 };
  ParametersValueType m_BlendingWeight{ 0.75 };
  bool                m_UseNoRigid{ true };
  unsigned int        m_Iterations{ 3 };
  bool                m_KeepTransforms{ false };

  std::vector<ParametersValueType>         m_Weights;
  std::vector<typename ImageType::Pointer> m_ImageList;
  typename PairwiseType::Pointer           m_PairwiseRegistration{ nullptr };

  std::vector<typename OutputTransformType::Pointer> m_TransformList;

#ifdef ITK_USE_CONCEPT_CHECKING
  static_assert(TImage::ImageDimension == TTemplateImage::ImageDimension,
                "Template imagemust have the same dimension as the input images.");
  static_assert(ImageDimension >= 2, "Images must be at least two-dimensional.");
#endif
};
} // namespace itk

#ifndef ITK_MANUAL_INSTANTIATION
#  include "itkANTsGroupwiseRegistration.hxx"
#endif

#endif // itkANTsGroupwiseRegistration
