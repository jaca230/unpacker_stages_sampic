#ifndef BYTE_STREAM_TO_SAMPIC_EVENT_STAGE_H
#define BYTE_STREAM_TO_SAMPIC_EVENT_STAGE_H

#include "analysis_pipeline/unpacker_core/stages/byte_stream_processor_stage.h"

class ByteStreamToSampicEventStage : public ByteStreamProcessorStage {
public:
    ByteStreamToSampicEventStage();
    ~ByteStreamToSampicEventStage() override = default;

    void OnInit() override;
    void Process() override;

    std::string Name() const override { return "ByteStreamToSampicEventStage"; }

private:
    std::string output_product_name_ = "SampicEvent";

    ClassDefOverride(ByteStreamToSampicEventStage, 1);
};

#endif // BYTE_STREAM_TO_SAMPIC_EVENT_STAGE_H
