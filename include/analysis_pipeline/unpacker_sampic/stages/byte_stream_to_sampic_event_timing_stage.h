#ifndef BYTE_STREAM_TO_SAMPIC_EVENT_TIMING_STAGE_H
#define BYTE_STREAM_TO_SAMPIC_EVENT_TIMING_STAGE_H

#include "analysis_pipeline/unpacker_core/stages/byte_stream_processor_stage.h"

class ByteStreamToSampicEventTimingStage : public ByteStreamProcessorStage {
public:
    ByteStreamToSampicEventTimingStage();
    ~ByteStreamToSampicEventTimingStage() override = default;

    void OnInit() override;
    void Process() override;

    std::string Name() const override { return "ByteStreamToSampicEventTimingStage"; }

private:
    std::string output_product_name_ = "SampicEventTiming";

    ClassDefOverride(ByteStreamToSampicEventTimingStage, 1);
};

#endif // BYTE_STREAM_TO_SAMPIC_EVENT_TIMING_STAGE_H
