#ifndef BYTE_STREAM_TO_SAMPIC_TRIGGER_GROUP_STAGE_H
#define BYTE_STREAM_TO_SAMPIC_TRIGGER_GROUP_STAGE_H

#include "analysis_pipeline/unpacker_core/stages/byte_stream_processor_stage.h"

class ByteStreamToSampicTriggerGroupStage : public ByteStreamProcessorStage {
public:
    void OnInit() override;
    void Process() override;

    std::string Name() const override { return "ByteStreamToSampicTriggerGroupStage"; }

private:
    std::string output_product_name_ = "SampicTriggerGroup";

    ClassDefOverride(ByteStreamToSampicTriggerGroupStage, 1);
};

#endif // BYTE_STREAM_TO_SAMPIC_TRIGGER_GROUP_STAGE_H
