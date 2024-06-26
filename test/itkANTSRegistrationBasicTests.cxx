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

#include "itkANTSRegistration.h"

#include "itkCommand.h"
#include "itkImageFileWriter.h"
#include "itkSimpleFilterWatcher.h"
#include "itkImageRegionIterator.h"
#include "itkSignedMaurerDistanceMapImageFilter.h"
#include "itkTxtTransformIOFactory.h"
#include "itkTestingMacros.h"

namespace
{
template <typename TImage>
void
setRegionToValue(TImage * image, const typename TImage::RegionType region, typename TImage::PixelType value)
{
  itk::ImageRegionIterator<TImage> imageIterator(image, region);
  while (!imageIterator.IsAtEnd())
  {
    imageIterator.Set(value);
    ++imageIterator;
  }
};

template <typename TResultImage, typename TInputImage>
typename TResultImage::Pointer
makeSDF(TInputImage * mask)
{
  using FloatImage = itk::Image<float, TInputImage::ImageDimension>;
  using DistanceMapFilterType = itk::SignedMaurerDistanceMapImageFilter<TInputImage, FloatImage>;
  typename DistanceMapFilterType::Pointer distanceMapFilter = DistanceMapFilterType::New();
  distanceMapFilter->SetInput(mask);
  distanceMapFilter->SetSquaredDistance(false);
  distanceMapFilter->SetUseImageSpacing(true);
  distanceMapFilter->SetInsideIsPositive(true);
  distanceMapFilter->Update();
  if constexpr (std::is_same_v<FloatImage, TResultImage>)
  {
    return distanceMapFilter->GetOutput();
  }
  // else we need to cast

  using CastFilterType = itk::CastImageFilter<FloatImage, TResultImage>;
  typename CastFilterType::Pointer castFilter = CastFilterType::New();
  castFilter->SetInput(distanceMapFilter->GetOutput());
  castFilter->Update();
  return castFilter->GetOutput();
}

template <typename FixedPixelType, typename MovingPixelType, unsigned Dimension>
int
testFilter(std::string outDir, std::string transformType)
{
  std::cout << "\n\n\nTesting: " << transformType << " " << Dimension << "D, ";
  std::cout << typeid(FixedPixelType).name() << "-" << typeid(MovingPixelType).name() << std::endl;

  using FixedImageType = itk::Image<FixedPixelType, Dimension>;
  using MovingImageType = itk::Image<MovingPixelType, Dimension>;
  using FilterType = itk::ANTSRegistration<FixedImageType, MovingImageType>;
  typename FilterType::Pointer filter = FilterType::New();
  ITK_EXERCISE_BASIC_OBJECT_METHODS(filter, ANTSRegistration, ProcessObject);

  using LabelImageType = itk::Image<unsigned char, Dimension>;

  // Create input masks to avoid test dependencies.
  typename LabelImageType::SizeType size;
  size.Fill(16);
  size[0] = 64;
  size[1] = 32;
  // 64x32 along x and y, 16 along other dimensions (z, t, etc)
  typename LabelImageType::SizeType shrinkRadius{ 0 };
  shrinkRadius[0] = 20;
  shrinkRadius[1] = 10;
  using PointType = itk::Point<double, Dimension>;
  PointType origin{ { -10.0, -5.0 } };

  typename LabelImageType::Pointer fixedMask = LabelImageType::New();
  fixedMask->SetRegions(size);
  fixedMask->Allocate();
  fixedMask->FillBuffer(0);
  fixedMask->SetOrigin(origin);
  typename LabelImageType::RegionType region = fixedMask->GetLargestPossibleRegion();
  region.ShrinkByRadius(shrinkRadius);
  setRegionToValue(fixedMask.GetPointer(), region, 1);
  typename FixedImageType::Pointer fixedImage = makeSDF<FixedImageType>(fixedMask.GetPointer());
  itk::WriteImage(fixedImage, outDir + "/SyntheticFixedSDF.nrrd");
  itk::WriteImage(fixedMask, outDir + "/SyntheticFixed-label.nrrd");

  typename LabelImageType::Pointer movingMask = LabelImageType::New();
  movingMask->SetRegions(size);
  movingMask->Allocate();
  movingMask->FillBuffer(0);
  origin[0] = 5;
  movingMask->SetOrigin(origin);
  region.SetIndex(0, region.GetIndex(0) + 5); // shift the rectangle
  setRegionToValue(movingMask.GetPointer(), region, 1);
  typename MovingImageType::Pointer movingImage = makeSDF<MovingImageType>(movingMask.GetPointer());
  itk::WriteImage(movingImage, outDir + "/SyntheticMovingSDF.nrrd");
  itk::WriteImage(movingMask, outDir + "/SyntheticMoving-label.nrrd");

  itk::SimpleFilterWatcher watcher(filter, "ANTs registration");

  filter->SetFixedImage(fixedImage);
  filter->SetMovingImage(movingImage);
  filter->SetFixedMask(fixedMask);
  filter->SetMovingMask(movingMask);
  filter->SetTypeOfTransform(transformType);
  filter->SetAffineMetric("MeanSquares");
  filter->SetSynMetric("MeanSquares");
  filter->SetCollapseCompositeTransform(true);
  filter->SetMaskAllStages(true);
  filter->SetSamplingRate(0.2);
  filter->SetRandomSeed(30101983);

  auto initialTransform = itk::TranslationTransform<double, Dimension>::New();
  using VectorType = itk::Vector<double, Dimension>;
  VectorType translation{ { 30.0, -5.0 } };
  initialTransform->Translate(translation);
  filter->SetInitialTransform(initialTransform.GetPointer());

  filter->DebugOn();
  filter->Update();

  auto forwardTransform = filter->GetForwardTransform();

  itk::TransformFileWriter::Pointer transformWriter = itk::TransformFileWriter::New();
  transformWriter->SetFileName(outDir + "/SyntheticForwardTransform.tfm");
  transformWriter->SetInput(forwardTransform);
  transformWriter->Update();

  typename MovingImageType::Pointer movingResampled = filter->GetWarpedMovingImage();
  itk::WriteImage(movingResampled, outDir + "/SyntheticMovingResampled.nrrd");

  auto inverseTransform = filter->GetInverseTransform(); // This should be invertible
  transformWriter->SetFileName(outDir + "/SyntheticInverseTransform.tfm");
  transformWriter->SetInput(inverseTransform);
  transformWriter->Update();
  std::cout << "\ninverseTransform: " << *inverseTransform << std::endl;

  typename FixedImageType::Pointer fixedResampled = filter->GetWarpedFixedImage();
  itk::WriteImage(fixedResampled, outDir + "/SyntheticFixedResampled.nrrd");

  // Check that the transform and the inverse are correct
  PointType zeroPoint{ { 0, 0 } };
  PointType transformedPoint = forwardTransform->TransformPoint(zeroPoint);
  // We expect the translation of 20 along i, and 0 along j (and k) axes
  PointType expectedPoint{ { 20, 0 } };
  for (unsigned d = 0; d < Dimension; ++d)
  {
    if (std::abs(transformedPoint[d] - expectedPoint[d]) > 0.5)
    {
      std::cerr << "Translation does not match expectation at dimension " << d << std::endl;
      std::cerr << "Expected: " << expectedPoint[d] << std::endl;
      std::cerr << "Got: " << transformedPoint[d] << std::endl;
      return EXIT_FAILURE;
    }
  }

  transformedPoint = inverseTransform->TransformPoint(zeroPoint);
  for (unsigned d = 0; d < Dimension; ++d)
  {
    if (std::abs(transformedPoint[d] + expectedPoint[d]) > 0.5)
    {
      std::cerr << "Translation in inverse transform does not match expectation at dimension " << d << std::endl;
      std::cerr << "Expected: " << -expectedPoint[d] << std::endl;
      std::cerr << "Got: " << transformedPoint[d] << std::endl;
      return EXIT_FAILURE;
    }
  }

  if (forwardTransform->IsLinear()) // Linear transforms should be combined into one
  {
    ITK_TEST_EXPECT_EQUAL(forwardTransform->GetNumberOfTransforms(), 1);
  }

  return EXIT_SUCCESS;
}
} // namespace


int
itkANTSRegistrationBasicTests(int argc, char * argv[])
{
  if (argc < 2)
  {
    std::cerr << "Missing parameters." << std::endl;
    std::cerr << "Usage: " << itkNameOfTestExecutableMacro(argv);
    std::cerr << " outputDirectory";
    std::cerr << std::endl;
    return EXIT_FAILURE;
  }

  itk::TxtTransformIOFactory::RegisterOneFactory();

  int overallSuccess = EXIT_SUCCESS;
  int retVal = EXIT_SUCCESS;

  retVal = testFilter<float, short, 3>(argv[1], "TRSAA");
  if (retVal != EXIT_SUCCESS)
  {
    overallSuccess = retVal;
  }

  retVal = testFilter<float, float, 3>(argv[1], "Rigid");
  if (retVal != EXIT_SUCCESS)
  {
    overallSuccess = retVal;
  }

  retVal = testFilter<short, short, 2>(argv[1], "Similarity");
  if (retVal != EXIT_SUCCESS)
  {
    overallSuccess = retVal;
  }

  retVal = testFilter<short, short, 4>(argv[1], "Translation");
  if (retVal != EXIT_SUCCESS)
  {
    overallSuccess = retVal;
  }

  retVal = testFilter<short, short, 3>(argv[1], "SyNRA");
  if (retVal != EXIT_SUCCESS)
  {
    overallSuccess = retVal;
  }

  return overallSuccess;
}
