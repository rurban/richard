#include "neural_net.hpp"
#include "utils.hpp"
#include "config.hpp"

namespace richard {

Hyperparams::Hyperparams()
  : epochs(0)
  , batchSize(1000)
  , miniBatchSize(16) {}

Hyperparams::Hyperparams(const Config& config) {
  epochs = config.getInteger("epochs");
  batchSize = config.getInteger("batchSize");
  miniBatchSize = config.getInteger("miniBatchSize");
}

const Config& Hyperparams::exampleConfig() {
  static Config config;
  static bool done = false;

  if (!done) {
    config.setInteger("epochs", 10);
    config.setInteger("batchSize", 1000);
    config.setInteger("miniBatchSize", 16);

    done = true;
  }

  return config;
}

const Config& NeuralNet::exampleConfig() {
  static Config config;
  static bool done = false;

  if (!done) {
    Config layer1;

    layer1.setString("type", "dense");
    layer1.setInteger("size", 300);
    layer1.setFloat("learnRate", 0.7);
    layer1.setFloat("learnRateDecay", 1.0);
    layer1.setFloat("dropoutRate", 0.5);

    Config layer2;
    layer2.setString("type", "dense");
    layer2.setInteger("size", 80);
    layer2.setFloat("learnRate", 0.7);
    layer2.setFloat("learnRateDecay", 1.0);
    layer2.setFloat("dropoutRate", 0.5);

    std::vector<Config> layersConfig{layer1, layer2};

    config.setObject("hyperparams", Hyperparams::exampleConfig());
    config.setObjectArray("hiddenLayers", layersConfig);

    Config outLayer;
    outLayer.setString("type", "output");
    outLayer.setInteger("size", 10);
    outLayer.setFloat("learnRate", 0.7);
    outLayer.setFloat("learnRateDecay", 1.0);

    config.setObject("outputLayer", outLayer);

    done = true;
  }

  return config;
}

}
