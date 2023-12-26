#pragma once

#include "cpu/layer.hpp"
#include <nlohmann/json.hpp>
#include <vector>

namespace richard {
namespace cpu {

class ConvolutionalLayer : public Layer {
  public:
    struct Filter {
      Filter()
        : K(1, 1, 1)
        , b(0.0) {}

      Kernel K;
      netfloat_t b;
    };

    ConvolutionalLayer(const nlohmann::json& obj, size_t inputW, size_t inputH, size_t inputDepth);
    ConvolutionalLayer(const nlohmann::json& obj, std::istream& stream, size_t inputW,
      size_t inputH, size_t inputDepth);

    LayerType type() const override { return LayerType::CONVOLUTIONAL; }
    Triple outputSize() const override;
    const DataArray& activations() const override;
    const DataArray& delta() const override;
    void trainForward(const DataArray& inputs) override;
    DataArray evalForward(const DataArray& inputs) const override;
    void updateDelta(const DataArray& inputs, const Layer& nextLayer) override;
    void updateParams(size_t epoch) override;
    void writeToStream(std::ostream& stream) const override;
    // Don't use. Use filters() instead.
    const Matrix& W() const override;
    const std::vector<Filter>& filters() const;

    std::array<size_t, 2> kernelSize() const;
    size_t depth() const;

    // Exposed for testing
    //

    void forwardPass(const Array3& inputs, Array3& Z) const;
    void setFilters(const std::vector<ConvolutionalLayer::Filter>& filters);

  private:
    std::vector<Filter> m_filters;
    Array3 m_Z;
    Array3 m_A;
    Array3 m_delta;
    std::vector<Filter> m_paramDeltas;
    size_t m_inputW;
    size_t m_inputH;
    size_t m_inputDepth;
    netfloat_t m_learnRate;
    netfloat_t m_learnRateDecay;
    netfloat_t m_dropoutRate;

    size_t numOutputs() const;
};

}
}