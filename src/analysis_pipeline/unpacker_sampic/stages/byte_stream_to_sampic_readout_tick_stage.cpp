#include "analysis_pipeline/unpacker_sampic/stages/byte_stream_to_sampic_readout_tick_stage.h"
#include "analysis_pipeline/unpacker_sampic/data_products/SampicReadoutTick.h"
#include "analysis_pipeline/unpacker_core/data_products/ByteStream.h"
#include "analysis_pipeline/core/data/pipeline_data_product.h"
#include <spdlog/spdlog.h>
#include <cstring>
#include <memory>
ClassImp(ByteStreamToSampicReadoutTickStage);
void ByteStreamToSampicReadoutTickStage::OnInit() {
    ByteStreamProcessorStage::OnInit();
    output_product_name_ = parameters_.value("output_product_name", "SampicReadoutTick");
}
void ByteStreamToSampicReadoutTickStage::Process() {
    auto input_lock = getInputByteStreamLock();
    if (!input_lock) return;
    auto bytes = std::dynamic_pointer_cast<dataProducts::ByteStream>(input_lock->getSharedObject());
    using Record = dataProducts::SampicReadoutTickRecord;
    if (!bytes || !bytes->data || bytes->size != sizeof(Record)) {
        spdlog::error("[{}] Invalid SR bank length (expected 56 bytes)", Name());
        return;
    }
    Record record{};
    std::memcpy(&record, bytes->data, sizeof(record));
    if (record.format_version != 1 || record.record_size != sizeof(Record)) {
        spdlog::error("[{}] Unsupported SR version/record size", Name());
        return;
    }
    auto result = std::make_unique<dataProducts::SampicReadoutTick>();
    result->format_version = record.format_version;
    result->record_size = record.record_size;
    result->flags = record.flags;
    result->first_tick = record.first_tick;
    result->ticks_elapsed = record.ticks_elapsed;
    result->start_ns = record.start_ns;
    result->end_ns = record.end_ns;
    result->actual_readout_ns = record.actual_readout_ns;
    result->hit_count = record.hit_count;
    result->parent_count = record.parent_count;
    auto product = std::make_unique<PipelineDataProduct>();
    product->setName(output_product_name_);
    product->setObject(std::move(result));
    product->addTag("sampic_readout_tick");
    product->addTag("built_by_byte_stream_to_sampic_readout_tick_stage");
    getDataProductManager()->addOrUpdate(output_product_name_, std::move(product));
}
