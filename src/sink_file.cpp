#include "sink.h"

#include <cstdio>
#include <string>
#include <cstdint>

class FilePcmSink : public IPcmSink {
public:
    explicit FilePcmSink(const std::string& path) {
        fp_ = std::fopen(path.c_str(), "ab");
    }
    ~FilePcmSink() {
        if (fp_) std::fclose(fp_);
    }
    void write(const int16_t* pcm, size_t samples) override {
        if (!fp_) return;
        std::fwrite(pcm, sizeof(int16_t), samples, fp_);
        if ((++cnt_ % 50) == 0) std::fflush(fp_);
    }
private:
    FILE* fp_{};
    size_t cnt_{0};
};

IPcmSink* makeFilePcmSink(const std::string& p) { return new FilePcmSink(p); }
