#ifndef BYTE_STREAM_TO_SAMPIC_TIME_STAGE_H
#define BYTE_STREAM_TO_SAMPIC_TIME_STAGE_H

#include "analysis_pipeline/unpacker_core/stages/byte_stream_processor_stage.h"

class ByteStreamToSampicTimeStage : public ByteStreamProcessorStage {
public:
    ByteStreamToSampicTimeStage();
    ~ByteStreamToSampicTimeStage() override = default;

    void OnInit() override;
    void Process() override;

    std::string Name() const override { return "ByteStreamToSampicTimeStage"; }

private:
    std::string output_product_name_;

    ClassDefOverride(ByteStreamToSampicTimeStage, 1);
};

#endif  // BYTE_STREAM_TO_SAMPIC_TIME_STAGE_H
