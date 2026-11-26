#include "analysis_pipeline/unpacker_sampic/stages/byte_stream_to_sampic_builder_event_stage.h"
#include "analysis_pipeline/unpacker_sampic/utils/builder_bank_decoder.h"
#include "analysis_pipeline/unpacker_core/data_products/ByteStream.h"
#include "analysis_pipeline/core/data/pipeline_data_product.h"
#include <spdlog/spdlog.h>
ClassImp(ByteStreamToSampicBuilderEventStage);
void ByteStreamToSampicBuilderEventStage::OnInit() {
    ByteStreamProcessorStage::OnInit();
    output_product_name_=parameters_.value("output_product_name", "SampicBuilderEvent");
}
void ByteStreamToSampicBuilderEventStage::Process() {
    auto lock=getInputByteStreamLock();
    if (!lock) return;
    auto bytes=std::dynamic_pointer_cast<dataProducts::ByteStream>(lock->getSharedObject());
    try {
        if (!bytes) throw std::runtime_error("invalid BD byte stream");
        auto result=std::make_unique<dataProducts::SampicBuilderEvent>(sampicBuilder::decodeEvent(bytes->data,bytes->size));
        auto product=std::make_unique<PipelineDataProduct>();
        product->setName(output_product_name_); product->setObject(std::move(result));
        product->addTag("sampic_builder_event");
        getDataProductManager()->addOrUpdate(output_product_name_,std::move(product));
    } catch (const std::exception& error) { spdlog::error("[{}] {}",Name(),error.what()); }
}
