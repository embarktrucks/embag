#include <lz4frame.h>

// LZ4 decompression context singleton
class Lz4DecompressionCtx {
  LZ4F_decompressionContext_t ctx_{nullptr};

  Lz4DecompressionCtx() {
    const auto code = LZ4F_createDecompressionContext(&ctx_, LZ4F_VERSION);
    if (LZ4F_isError(code)) {
      throw std::runtime_error("Received error code from LZ4F_createDecompressionContext: " + std::to_string(code));
    }
  }

 public:
  static Lz4DecompressionCtx& getInstance() {
    static Lz4DecompressionCtx instance;
    return instance;
  }

  LZ4F_decompressionContext_t context() {
    return this->ctx_;
  }

  // Ensure we don't copy this singleton
  Lz4DecompressionCtx(Lz4DecompressionCtx const&) = delete;
  void operator=(Lz4DecompressionCtx const&) = delete;
};
