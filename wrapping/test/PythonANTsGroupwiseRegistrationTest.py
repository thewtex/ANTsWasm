#==========================================================================
#
#   Copyright NumFOCUS
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#          https://www.apache.org/licenses/LICENSE-2.0.txt
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
#
#==========================================================================*/


import itk
import argparse

parser = argparse.ArgumentParser(description="Python groupwise registration test.")
parser.add_argument("-i", "--input-image", action="append", help="Input image(s)", required=True)
parser.add_argument("-t", "--initial-template", required=False)
parser.add_argument("-o", "--output-template", required=True)
args = parser.parse_args()

itk.auto_progress(2)

first_image = itk.imread(args.input_image[0])

PixelType = itk.template(first_image)[1][0]
Dimension = first_image.GetImageDimension()
ImageType = itk.Image[PixelType, Dimension]
assert ImageType == type(first_image)
TemplateImageType = itk.Image[itk.F, Dimension]

gwr = itk.ANTsGroupwiseRegistration[ImageType, TemplateImageType, itk.F].New()
# pairwise registration has different order of template parameters
pwr = itk.ANTsRegistration[TemplateImageType, ImageType, itk.F].New()

if args.initial_template is not None:
    print(f"Reading initial template: {args.initial_template}")
    initial_template = itk.imread(args.initial_template)
    gwr.SetInitialTemplateImage(initial_template)

images = []
for i, input_filename in enumerate(args.input_image):
    print(f"Reading {input_filename}")
    image = itk.imread(input_filename)
    images.append(image)
gwr.SetImageList(images)

gwr.SetIterations(4)
gwr.SetGradientStep(0.15)

pwr.SetTypeOfTransform("SyN")  # example used BSplineSyN, but we have to choose BSpline OR SyN
pwr.SetSynMetric("CC")
pwr.SetSynIterations([100, 100, 100, 70, 50, 10])
pwr.SetShrinkFactors([16, 12, 8, 4, 2, 1])
pwr.SetSmoothingSigmas([4, 4, 4, 2, 1, 0])

gwr.SetPairwiseRegistration(pwr)
print(gwr)

print("Performing groupwise registration. This will take a while...")
gwr.Update()

print(f"Writing resulting template image into file: {args.output_template}")
itk.imwrite(gwr.GetOutput(), args.output_template, compression=False)

print("Test finished")
