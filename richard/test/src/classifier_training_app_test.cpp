#include "mock_file_system.hpp"
#include "mock_logger.hpp"
#include "mock_platform_paths.hpp"
#include <classifier_training_app.hpp>
#include <gtest/gtest.h>

using namespace richard;
using testing::NiceMock;

class ClassifierTrainingAppTest : public testing::Test {
  public:
    virtual void SetUp() override {}
    virtual void TearDown() override {}
};

TEST_F(ClassifierTrainingAppTest, exampleConfig) {
  Config config = ClassifierTrainingApp::exampleConfig();

  NiceMock<MockFileSystem> fileSystem;
  NiceMock<MockPlatformPaths> platformPaths;
  NiceMock<MockLogger> logger;

  ClassifierTrainingApp::Options opts;
  opts.samplesPath = "samples.csv";
  opts.configFile = "config.json";
  opts.networkFile = "savefile";

  std::unique_ptr<std::istream> samplesStream = std::make_unique<std::stringstream>("1,0,255,128");
  std::unique_ptr<std::istream> configStream = std::make_unique<std::stringstream>(config.dump());
  std::unique_ptr<std::ostream> saveFileStream = std::make_unique<std::stringstream>();

  ON_CALL(fileSystem, openFileForReading(std::filesystem::path("samples.csv")))
    .WillByDefault(testing::Return(testing::ByMove(std::move(samplesStream))));

  ON_CALL(fileSystem, openFileForReading(std::filesystem::path("config.json")))
    .WillByDefault(testing::Return(testing::ByMove(std::move(configStream))));

  ON_CALL(fileSystem, openFileForWriting(std::filesystem::path("savefile")))
    .WillByDefault(testing::Return(testing::ByMove(std::move(saveFileStream))));

  ClassifierTrainingApp app(fileSystem, platformPaths, opts, logger);
}
