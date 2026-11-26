#pragma once
#include "analysis_pipeline/unpacker_core/stages/byte_stream_processor_stage.h"
class ByteStreamToSampicBuilderEventStage : public ByteStreamProcessorStage {
public:
    void OnInit() override;
    void Process() override;
    std::string Name() const override { return "ByteStreamToSampicBuilderEventStage"; }
private:
    std::string output_product_name_ = "SampicBuilderEvent";
    ClassDefOverride(ByteStreamToSampicBuilderEventStage, 1);
};
