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

#include "itkANTsGroupwiseRegistration.h"

#include "itkImageFileWriter.h"
#include "itkSimpleFilterWatcher.h"
#include "itkTxtTransformIOFactory.h"
#include "itkTestingMacros.h"


int
itkANTsGroupwiseRegistrationTest(int argc, char * argv[])
{
  if (argc < 3)
  {
    std::cerr << "Missing parameters." << std::endl;
    std::cerr << "Usage: " << itkNameOfTestExecutableMacro(argv);
    std::cerr << " inputDirectory outputTemplateName [numberOfFaces] [typeOfTransform] [initialTemplate]";
    std::cerr << std::endl;
    return EXIT_FAILURE;
  }

  itk::TxtTransformIOFactory::RegisterOneFactory();

  std::string firstArg = argv[1];
  std::string inDir = firstArg.substr(0, firstArg.size() - 11); // cut off /face10.png
  std::string outName = argv[2];

  unsigned numberOfFaces = 10;
  if (argc > 3)
  {
    numberOfFaces = std::stoul(argv[3]);
  }

  std::string typeOfTransform = "SyN";
  if (argc > 4)
  {
    typeOfTransform = argv[4];
  }

  using ImageType = itk::Image<unsigned char, 2>;
  using FloatImageType = itk::Image<float, 2>;
  using FilterType = itk::ANTsGroupwiseRegistration<ImageType, FloatImageType, float>;
  typename FilterType::Pointer filter = FilterType::New();

  if (argc > 5)
  {
    auto initialTemplate = itk::ReadImage<FloatImageType>(argv[5]);
    filter->SetInitialTemplateImage(initialTemplate);
  }
  ITK_EXERCISE_BASIC_OBJECT_METHODS(filter, ANTsGroupwiseRegistration, ImageToImageFilter);

  itk::SimpleFilterWatcher        watcher(filter, "ANTs groupwise registration");
  std::vector<ImageType::Pointer> images;
  for (unsigned i = 1; i <= numberOfFaces; ++i)
  {
    std::string fileName = inDir + "/face" + std::to_string(i) + ".png";
    auto        image = itk::ReadImage<ImageType>(fileName);
    images.push_back(image);
  }

  filter->SetImageList(images);
  filter->SetIterations(4);
  filter->SetGradientStep(0.15);

  using PairwiseType = itk::ANTsRegistration<FloatImageType, ImageType, float>;
  typename PairwiseType::Pointer pairwiseRegistration = PairwiseType::New();
  pairwiseRegistration->SetTypeOfTransform(typeOfTransform);
  pairwiseRegistration->SetSynMetric("CC");
  pairwiseRegistration->SetAffineMetric("CC");
  std::vector<unsigned> iterations{ 100, 100, 100, 70, 50, 10 };
  pairwiseRegistration->SetAffineIterations(iterations);
  pairwiseRegistration->SetSynIterations(iterations);
  pairwiseRegistration->SetShrinkFactors({ 16, 12, 8, 4, 2, 1 });
  pairwiseRegistration->SetSmoothingSigmas({ 4, 4, 4, 2, 1, 0 });
  pairwiseRegistration->SetRandomSeed(19831030); // helps with reproducibility
  filter->SetPairwiseRegistration(pairwiseRegistration);

  filter->DebugOn();
  filter->KeepTransformsOn();
  filter->Update();

  auto templateImage = filter->GetTemplateImage();
  std::cout << "templateImage: " << *templateImage;
  itk::WriteImage(templateImage, outName);

  using TransformWriter = itk::TransformFileWriterTemplate<float>;
  TransformWriter::Pointer transformWriter = TransformWriter::New();
  for (unsigned i = 0; i < numberOfFaces; ++i)
  {
    auto fTransform = filter->GetTransform(i);
    transformWriter->SetFileName(outName + "_" + std::to_string(i + 1) + ".tfm");
    transformWriter->SetInput(fTransform);
    transformWriter->Update();
  }

  return EXIT_SUCCESS;
}
