#pragma once
#include "analysis_pipeline/core/stages/base_stage.h"
#include <vector>
class ByteStreamsToSampicBuilderDiagnosticsStage : public BaseStage {
public:
    void OnInit() override;
    void Process() override;
    std::string Name() const override { return "ByteStreamsToSampicBuilderDiagnosticsStage"; }
private:
    std::string output_product_name_ = "SampicBuilderDiagnostics";
    std::string input_prefix_ = "bytestream_bank_";
    std::vector<std::string> bank_names_;
    ClassDefOverride(ByteStreamsToSampicBuilderDiagnosticsStage, 1);
};
