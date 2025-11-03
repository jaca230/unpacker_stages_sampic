#ifndef BYTE_STREAM_TO_SAMPIC_COLLECTOR_TIMING_STAGE_H
#define BYTE_STREAM_TO_SAMPIC_COLLECTOR_TIMING_STAGE_H

#include "analysis_pipeline/unpacker_core/stages/byte_stream_processor_stage.h"

class ByteStreamToSampicCollectorTimingStage : public ByteStreamProcessorStage {
public:
    ByteStreamToSampicCollectorTimingStage();
    ~ByteStreamToSampicCollectorTimingStage() override = default;

    void OnInit() override;
    void Process() override;

    std::string Name() const override { return "ByteStreamToSampicCollectorTimingStage"; }

private:
    std::string output_product_name_ = "SampicCollectorTiming";

    ClassDefOverride(ByteStreamToSampicCollectorTimingStage, 1);
};

#endif // BYTE_STREAM_TO_SAMPIC_COLLECTOR_TIMING_STAGE_H
