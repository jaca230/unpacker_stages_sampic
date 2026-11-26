#include "analysis_pipeline/unpacker_sampic/stages/byte_streams_to_sampic_builder_diagnostics_stage.h"
#include "analysis_pipeline/unpacker_sampic/utils/builder_bank_decoder.h"
#include "analysis_pipeline/unpacker_core/data_products/ByteStream.h"
#include "analysis_pipeline/core/data/pipeline_data_product.h"
#include <spdlog/spdlog.h>
ClassImp(ByteStreamsToSampicBuilderDiagnosticsStage);
void ByteStreamsToSampicBuilderDiagnosticsStage::OnInit() {
    output_product_name_=parameters_.value("output_product_name", "SampicBuilderDiagnostics");
    input_prefix_=parameters_.value("input_prefix", "bytestream_bank_");
    bank_names_=parameters_.value("bank_names",std::vector<std::string>{"BT00","BT01","BC00","BS00"});
}
void ByteStreamsToSampicBuilderDiagnosticsStage::Process() {
    auto result=std::make_unique<dataProducts::SampicBuilderDiagnostics>();
    bool found=false;
    try {
        for (const auto& name:bank_names_) {
            const auto key=input_prefix_+name;
            if (!getDataProductManager()->hasProduct(key)) continue;
            auto lock=getDataProductManager()->checkoutRead(key);
            if (!lock.get()) continue;
            auto bytes=std::dynamic_pointer_cast<dataProducts::ByteStream>(lock.get()->getSharedObject());
            if (!bytes) throw std::runtime_error("invalid builder timing byte stream");
            found=true;
            if (name=="BS00") {
                auto cuts=sampicBuilder::decodeCuts(bytes->data,bytes->size);
                result->cut_cycle_id=cuts.first; result->cut_summaries=std::move(cuts.second);
                result->has_cut_summary=true;
            } else if (name=="BC00") {
                result->collector_cycle=sampicBuilder::decodeCollector(bytes->data,bytes->size);
                result->has_collector_cycle=true;
            } else {
                if (name.size()!=4 || name.substr(0,2)!="BT" || name[2]<'0' || name[2]>'9' || name[3]<'0' || name[3]>'9')
                    throw std::runtime_error("invalid configured BT bank name");
                result->acquisition_summaries.push_back(sampicBuilder::decodeSummary(bytes->data,bytes->size,(name[2]-'0')*10+name[3]-'0'));
            }
        }
        if (!found) return;
        if (result->has_collector_cycle)
            for (const auto& summary:result->acquisition_summaries)
                if (summary.cycle_id!=result->collector_cycle.cycle_id)
                    throw std::runtime_error("BT/BC cycle ID mismatch");
        if (result->has_cut_summary) {
            if (result->has_collector_cycle && result->cut_cycle_id!=result->collector_cycle.cycle_id)
                throw std::runtime_error("BS/BC cycle ID mismatch");
            for (const auto& summary:result->acquisition_summaries)
                if (summary.cycle_id!=result->cut_cycle_id) throw std::runtime_error("BS/BT cycle ID mismatch");
        }
        auto product=std::make_unique<PipelineDataProduct>();
        product->setName(output_product_name_); product->setObject(std::move(result));
        product->addTag("sampic_builder_diagnostics");
        getDataProductManager()->addOrUpdate(output_product_name_,std::move(product));
    } catch (const std::exception& error) { spdlog::error("[{}] {}",Name(),error.what()); }
}
