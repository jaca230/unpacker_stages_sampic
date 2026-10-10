#ifndef BYTE_STREAM_TO_SAMPIC_HIT_METADATA_STAGE_H
#define BYTE_STREAM_TO_SAMPIC_HIT_METADATA_STAGE_H

#include "analysis_pipeline/unpacker_core/stages/byte_stream_processor_stage.h"

class ByteStreamToSampicHitMetadataStage : public ByteStreamProcessorStage {
public:
    void OnInit() override;
    void Process() override;

    std::string Name() const override { return "ByteStreamToSampicHitMetadataStage"; }

private:
    std::string output_product_name_ = "SampicHitMetadata";

    ClassDefOverride(ByteStreamToSampicHitMetadataStage, 1);
};

#endif // BYTE_STREAM_TO_SAMPIC_HIT_METADATA_STAGE_H
