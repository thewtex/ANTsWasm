# the top-level README is used for describing this module, just
# re-used it for documentation here
get_filename_component(MY_CURRENT_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)
file(READ "${MY_CURRENT_DIR}/README.rst" DOCUMENTATION)

# define the dependencies of the include module and the tests
itk_module(ANTsWasm
  DEPENDS
    ITKCommon
    ITKTransform
    ITKRegistrationMethodsv4  # ImageRegistrationMethodv4 wraps DataObjectDecorator<CompositeTransform>
    ITKIOImageBase
    ITKImageLabel
    ITKBinaryMathematicalMorphology
    ITKTransformFactory
    ITKIOTransformBase
    ITKImageGrid
    ITKDisplacementField
  TEST_DEPENDS
    ITKTestKernel
    ITKMetaIO
    ITKIONRRD
    ITKIONIFTI
    ITKIOTransformHDF5
    ITKIOTransformMatlab
    ITKIOTransformInsightLegacy
    ITKDistanceMap
  DESCRIPTION
    "${DOCUMENTATION}"
  EXCLUDE_FROM_DEFAULT
  ENABLE_SHARED
)
