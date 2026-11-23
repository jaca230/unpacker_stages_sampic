#pragma once
#include "analysis_pipeline/unpacker_core/stages/byte_stream_processor_stage.h"
class ByteStreamToSampicReadoutTickStage : public ByteStreamProcessorStage {
public:
    void OnInit() override;
    void Process() override;
    std::string Name() const override { return "ByteStreamToSampicReadoutTickStage"; }
private:
    std::string output_product_name_ = "SampicReadoutTick";
    ClassDefOverride(ByteStreamToSampicReadoutTickStage, 1);
};
