#ifndef BYTE_STREAM_TO_SAMPIC_TIMING_STAGE_H
#define BYTE_STREAM_TO_SAMPIC_TIMING_STAGE_H

#include "analysis_pipeline/unpacker_core/stages/byte_stream_processor_stage.h"

class ByteStreamToSampicTimingStage : public ByteStreamProcessorStage {
public:
    ByteStreamToSampicTimingStage();
    ~ByteStreamToSampicTimingStage() override = default;

    void OnInit() override;
    void Process() override;

    std::string Name() const override { return "ByteStreamToSampicTimingStage"; }

private:
    std::string output_product_name_ = "SampicTiming";

    ClassDefOverride(ByteStreamToSampicTimingStage, 1);
};

#endif // BYTE_STREAM_TO_SAMPIC_TIMING_STAGE_H
