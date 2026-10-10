#ifndef BYTE_STREAM_TO_SAMPIC_TRIGGER_RECORDS_STAGE_H
#define BYTE_STREAM_TO_SAMPIC_TRIGGER_RECORDS_STAGE_H

#include "analysis_pipeline/unpacker_core/stages/byte_stream_processor_stage.h"

class ByteStreamToSampicTriggerRecordsStage : public ByteStreamProcessorStage {
public:
    void OnInit() override;
    void Process() override;

    std::string Name() const override { return "ByteStreamToSampicTriggerRecordsStage"; }

private:
    std::string output_product_name_ = "SampicTriggerRecords";

    ClassDefOverride(ByteStreamToSampicTriggerRecordsStage, 1);
};

#endif // BYTE_STREAM_TO_SAMPIC_TRIGGER_RECORDS_STAGE_H
