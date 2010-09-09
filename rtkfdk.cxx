#include "rtkfdk_ggo.h"
#include "rtkThreeDCircularGeometryXMLFile.h"

#include "itkBackProjectionImageFilter.h"
#include "itkProjectionsReader.h"
#include "itkFFTRampImageFilter.h"

#include <itkImageFileWriter.h>
#include <itkRegularExpressionSeriesFileNames.h>
#include <itkTimeProbe.h>
#include <itkStreamingImageFilter.h>

int main(int argc, char * argv[])
{
  GGO(rtkfdk, args_info);

  typedef double OutputPixelType;
  const unsigned int Dimension = 3;

  typedef itk::Image< OutputPixelType, Dimension > OutputImageType;

  // Generate file names
  itk::RegularExpressionSeriesFileNames::Pointer names = itk::RegularExpressionSeriesFileNames::New();
  names->SetDirectory(args_info.path_arg);
  names->SetNumericSort(false);
  names->SetRegularExpression(args_info.regexp_arg);
  names->SetSubMatch(0);

  // Projections reader
  typedef itk::ProjectionsReader< OutputImageType > ReaderType;
  ReaderType::Pointer reader = ReaderType::New();
  reader->SetFileNames( names->GetFileNames() );
  reader->GenerateOutputInformation();

  // Ramp filter
  typedef itk::FFTRampImageFilter<OutputImageType> RampFilterType;
  RampFilterType::Pointer rampFilter = RampFilterType::New();
  rampFilter->SetInput( reader->GetOutput() );

  // Streaming filter
  typedef itk::StreamingImageFilter<OutputImageType, OutputImageType> StreamerType;
  StreamerType::Pointer streamer = StreamerType::New();
  streamer->SetInput( rampFilter->GetOutput() );
  streamer->SetNumberOfStreamDivisions( 1 + reader->GetOutput()->GetLargestPossibleRegion().GetNumberOfPixels() / (1024*1024*4) );

  // Try to do all 2D pre-processing
  try {
    streamer->Update();
  } catch( itk::ExceptionObject & err ) {
    std::cerr << "ExceptionObject caught !" << std::endl;
    std::cerr << err << std::endl;
    return EXIT_FAILURE;
  }

  // Geometry
  rtk::ThreeDCircularGeometryXMLFileReader::Pointer geometryReader = rtk::ThreeDCircularGeometryXMLFileReader::New();
  geometryReader->SetFilename(args_info.geometry_arg);
  geometryReader->GenerateOutputInformation();

  // Backprojection
  typedef itk::BackProjectionImageFilter<OutputImageType, OutputImageType> BackProjectionFilterType;
  BackProjectionFilterType::Pointer bpFilter = BackProjectionFilterType::New();
  bpFilter->SetInput( streamer->GetOutput() );
  bpFilter->SetGeometry( geometryReader->GetOutputObject() );
  bpFilter->SetFromGengetopt(args_info);

  // Write
  typedef itk::ImageFileWriter<  OutputImageType >  WriterType;
  WriterType::Pointer writer = WriterType::New();
  writer->SetFileName( args_info.output_arg );
  writer->SetInput( bpFilter->GetOutput() );

  try {
    writer->Update();
  } catch( itk::ExceptionObject & err ) {
    std::cerr << "ExceptionObject caught !" << std::endl;
    std::cerr << err << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}