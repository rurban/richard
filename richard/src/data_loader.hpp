#pragma once

#include "math.hpp"
#include "config.hpp"
#include <vector>
#include <memory>

namespace richard {

struct Sample {
  Sample(const std::string& label, const Array3& data)
    : label(label)
    , data(data) {}

  std::string label; // TODO: Replace with reference/pointer/id
  Array3 data;
};

class DataLoader {
  public:
    DataLoader(size_t fetchSize);

    virtual size_t loadSamples(std::vector<Sample>& samples) = 0;
    virtual void seekToBeginning() = 0;

    inline size_t fetchSize() const;

    virtual ~DataLoader() {}
    
    static const Config& exampleConfig();

  private:
    size_t m_fetchSize;
};

using DataLoaderPtr = std::unique_ptr<DataLoader>;

size_t DataLoader::fetchSize() const {
  return m_fetchSize;
}

class FileSystem;
class DataDetails;

DataLoaderPtr createDataLoader(FileSystem& fileSystem, const Config& config,
  const std::string& samplesPath, const DataDetails& dataDetails);

}
