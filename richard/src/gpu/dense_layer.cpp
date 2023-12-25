#include "gpu/dense_layer.hpp"
#include "util.hpp"

namespace richard {
namespace gpu {

DenseLayer::DenseLayer(Gpu& gpu, const nlohmann::json& obj, std::istream& stream, size_t inputSize)
  : m_gpu(gpu)
  , m_inputSize(inputSize) {

  m_size = getOrThrow(obj, "size").get<size_t>();
  m_learnRate = getOrThrow(obj, "learnRate").get<netfloat_t>();
  m_learnRateDecay = getOrThrow(obj, "learnRateDecay").get<netfloat_t>();
  m_dropoutRate = getOrThrow(obj, "dropoutRate").get<netfloat_t>();

  m_B = Vector(m_size);
  stream.read(reinterpret_cast<char*>(m_B.data()), m_size * sizeof(netfloat_t));

  m_W = Matrix(m_inputSize, m_size);
  stream.read(reinterpret_cast<char*>(m_W.data()), m_W.rows() * m_W.cols() * sizeof(netfloat_t));
}

DenseLayer::DenseLayer(Gpu& gpu, const nlohmann::json& obj, size_t inputSize)
  : m_gpu(gpu)
  , m_inputSize(inputSize) {

  m_size = getOrThrow(obj, "size").get<size_t>();
  m_learnRate = getOrThrow(obj, "learnRate").get<netfloat_t>();
  m_learnRateDecay = getOrThrow(obj, "learnRateDecay").get<netfloat_t>();
  m_dropoutRate = getOrThrow(obj, "dropoutRate").get<netfloat_t>();

  m_B = Vector(m_size);
  m_B.randomize(0.1);
  m_W = Matrix(m_inputSize, m_size);
  m_W.randomize(0.1);
}

void DenseLayer::allocateGpuResources(GpuBufferHandle inputBuffer, GpuBufferHandle statusBuffer,
  const Layer* nextLayer, GpuBufferHandle) {

  DBG_ASSERT(nextLayer != nullptr);

  GpuBufferFlags paramBuffersFlags = GpuBufferFlags::large
                                   | GpuBufferFlags::hostReadAccess
                                   | GpuBufferFlags::hostWriteAccess;

  m_bufferB = m_gpu.allocateBuffer(m_size * sizeof(netfloat_t), paramBuffersFlags);
  m_bufferW = m_gpu.allocateBuffer(m_inputSize * m_size * sizeof(netfloat_t), paramBuffersFlags);
  m_bufferZ = m_gpu.allocateBuffer(m_size * sizeof(netfloat_t), GpuBufferFlags::large);
  m_bufferA = m_gpu.allocateBuffer(m_size * sizeof(netfloat_t), GpuBufferFlags::large);
  m_bufferD = m_gpu.allocateBuffer(m_size * sizeof(netfloat_t), GpuBufferFlags::large);
  m_bufferDeltaB = m_gpu.allocateBuffer(m_size * sizeof(netfloat_t), GpuBufferFlags::large);
  m_bufferDeltaW = m_gpu.allocateBuffer(m_inputSize * m_size * sizeof(netfloat_t),
    GpuBufferFlags::large);

  m_gpu.submitBufferData(m_bufferB.handle, m_B.data());
  m_gpu.submitBufferData(m_bufferW.handle, m_W.data());

  GpuBufferBindings evalForwardBuffers{
    inputBuffer,
    m_bufferB.handle,
    m_bufferW.handle,
    m_bufferA.handle
  };

  GpuBufferBindings trainForwardBuffers{
    statusBuffer,
    inputBuffer,
    m_bufferB.handle,
    m_bufferW.handle,
    m_bufferZ.handle,
    m_bufferA.handle
  };

  GpuBufferBindings backpropBuffers{
    statusBuffer,
    inputBuffer,
    m_bufferB.handle,
    m_bufferW.handle,
    m_bufferZ.handle,
    m_bufferA.handle,
    m_bufferD.handle,
    nextLayer->weightsBuffer(),
    nextLayer->deltaBuffer(),
    m_bufferDeltaB.handle,
    m_bufferDeltaW.handle
  };

  GpuBufferBindings updateParamsBuffers{
    statusBuffer,
    m_bufferB.handle,
    m_bufferW.handle,
    m_bufferDeltaB.handle,
    m_bufferDeltaW.handle
  };

  Size3 workgroupSize{ static_cast<uint32_t>(m_size), 1, 1 };

  SpecializationConstants evalForwardConstants{
    { SpecializationConstant::Type::uint_type, static_cast<uint32_t>(m_inputSize) }
  };

  SpecializationConstants trainForwardConstants{
    { SpecializationConstant::Type::uint_type, static_cast<uint32_t>(m_inputSize) },
  //  { SpecializationConstant::Type::float_type, m_dropoutRate }
  };

  SpecializationConstants backpropConstants{
    { SpecializationConstant::Type::uint_type, static_cast<uint32_t>(m_inputSize) },
    { SpecializationConstant::Type::uint_type, static_cast<uint32_t>(nextLayer->size()) }
  };

  SpecializationConstants updateParamsConstants{
    { SpecializationConstant::Type::uint_type, static_cast<uint32_t>(m_inputSize) },
    { SpecializationConstant::Type::float_type, m_learnRate },
    { SpecializationConstant::Type::float_type, m_learnRateDecay },
  };

  m_gpu.compileShader("./shaders/dense_eval_forward.glsl", evalForwardBuffers,
    evalForwardConstants, workgroupSize);
  m_gpu.compileShader("./shaders/dense_train_forward.glsl", trainForwardBuffers,
    trainForwardConstants, workgroupSize);
  m_gpu.compileShader("./shaders/dense_backprop.glsl", backpropBuffers, backpropConstants,
    workgroupSize);
  m_gpu.compileShader("./shaders/dense_update_params.glsl", updateParamsBuffers,
    updateParamsConstants, workgroupSize);
}

size_t DenseLayer::size() const {
  return m_size;
}

Triple DenseLayer::outputSize() const {
  return { m_size, 1, 1 };
}

void DenseLayer::evalForward() {
  m_gpu.queueShader(m_evalForwardShader);
}

void DenseLayer::trainForward() {
  m_gpu.queueShader(m_trainForwardShader);
}

void DenseLayer::backprop() {
  m_gpu.queueShader(m_backpropShader);
}

void DenseLayer::updateParams() {
  m_gpu.queueShader(m_updateParamsShader);
}

GpuBufferHandle DenseLayer::outputBuffer() const {
  return m_bufferA.handle;
}

GpuBufferHandle DenseLayer::weightsBuffer() const {
  return m_bufferW.handle;
}

GpuBufferHandle DenseLayer::deltaBuffer() const {
  return m_bufferD.handle;
}

void DenseLayer::retrieveBuffers() {
  m_gpu.retrieveBuffer(m_bufferB.handle, m_B.data());
  m_gpu.retrieveBuffer(m_bufferW.handle, m_W.data());
}

void DenseLayer::writeToStream(std::ostream& stream) const {
  stream.write(reinterpret_cast<const char*>(m_B.data()), m_B.size() * sizeof(netfloat_t));
  stream.write(reinterpret_cast<const char*>(m_W.data()),
    m_W.rows() * m_W.cols() * sizeof(netfloat_t));
}

}
}
