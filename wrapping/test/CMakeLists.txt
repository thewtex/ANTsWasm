set(test_input_dir ${${itk-module}_SOURCE_DIR}/test/Input)
set(test_baseline_dir ${${itk-module}_SOURCE_DIR}/test/Baseline)

itk_python_expression_add_test(NAME PythonInstantiateANTsRegistrationFloat2D
  EXPRESSION "reg = itk.ANTsRegistration[itk.Image[itk.F, 2], itk.Image[itk.F, 2], itk.D].New()"
  )

itk_python_add_test(NAME PythonANTsRegistrationTest
  TEST_DRIVER_ARGS
  --compareIntensityTolerance 7
  --compareRadiusTolerance 0
  --compareNumberOfPixelsTolerance 777
  --compare
    DATA{${test_baseline_dir}/antsRegistrationTest_AffineTranslationNoMasks.result.nii.gz}
    ${ITK_TEST_OUTPUT_DIR}/PythonANTsRegistrationTest_AffineTranslationNoMasks.result.nii.gz
  COMMAND PythonANTsRegistrationTest.py
    --fixed-image DATA{${test_input_dir}/test.nii.gz}
    --moving-image DATA{${test_input_dir}/translation.test.nii.gz}
    --initial-transform DATA{${test_input_dir}/Initializer_antsRegistrationTest_AffineTranslationMasks.mat}
    --output-transform ${ITK_TEST_OUTPUT_DIR}/PythonANTsRegistrationTest_AffineTranslationNoMasks.tfm
    --resampled-moving ${ITK_TEST_OUTPUT_DIR}/PythonANTsRegistrationTest_AffineTranslationNoMasks.result.nii.gz
  )

itk_python_expression_add_test(NAME PythonInstantiateANTsGroupwiseRegistrationShort3Dfloat
  EXPRESSION "reg = itk.ANTsGroupwiseRegistration[itk.Image[itk.SS, 3], itk.Image[itk.F, 3], itk.F].New()"
  )

itk_python_add_test(NAME PythonANTsGroupwiseRegistrationTest
  TEST_DRIVER_ARGS
  --compare
    DATA{${test_baseline_dir}/T_template0Result337.nrrd}
    ${ITK_TEST_OUTPUT_DIR}/PythonANTsGroupwiseRegistrationTest.nrrd
  COMMAND PythonANTsGroupwiseRegistrationTest.py
    -o ${ITK_TEST_OUTPUT_DIR}/PythonANTsGroupwiseRegistrationTest.nrrd
    -i DATA{${test_input_dir}/face1.png}
    -i DATA{${test_input_dir}/face2.png}
    -i DATA{${test_input_dir}/face3.png}
    -i DATA{${test_input_dir}/face4.png}
    -i DATA{${test_input_dir}/face5.png}
    -i DATA{${test_input_dir}/face6.png}
    -i DATA{${test_input_dir}/face7.png}
    -i DATA{${test_input_dir}/face8.png}
    -i DATA{${test_input_dir}/face9.png}
    -i DATA{${test_input_dir}/face10.png}
  )
