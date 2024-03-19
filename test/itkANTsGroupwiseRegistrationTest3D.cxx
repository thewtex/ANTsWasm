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
#include "itkTxtTransformIOFactory.h"
#include "itkTestingMacros.h"


int
itkANTsGroupwiseRegistrationTest3D(int argc, char * argv[])
{
  if (argc < 2)
  {
    std::cerr << "Missing parameters." << std::endl;
    std::cerr << "Usage: " << itkNameOfTestExecutableMacro(argv);
    std::cerr << " inputDirectory outputDirectory";
    std::cerr << std::endl;
    return EXIT_FAILURE;
  }

  itk::TxtTransformIOFactory::RegisterOneFactory();

  std::string inDir = argv[1];
  std::string outDir = argv[2];

  unsigned numberOfImages = 3;

  using ImageType = itk::Image<signed short, 3>;
  using FloatImageType = itk::Image<float, 3>;
  using FilterType = itk::ANTsGroupwiseRegistration<ImageType, ImageType, float>;
  typename FilterType::Pointer filter = FilterType::New();

  std::vector<ImageType::Pointer> images;
  for (unsigned i = 1; i <= numberOfImages; ++i)
  {
    std::string fileName = inDir + "/lola11-0" + std::to_string(i) + ".nrrd";
    auto        image = itk::ReadImage<ImageType>(fileName);
    images.push_back(image);
  }

  filter->SetImageList(images);
  filter->SetInitialTemplateImage(images[images.size() / 2]);
  filter->DebugOn();
  filter->Update();

  auto templateImage = filter->GetTemplateImage();
  templateImage->DisconnectPipeline();
  itk::WriteImage(templateImage, outDir + "/chestTemplate.nrrd");

  return EXIT_SUCCESS;
}
