#include "analysis_pipeline/unpacker_sampic/stages/byte_stream_to_sampic_time_bin_stage.h"
#include "analysis_pipeline/unpacker_sampic/data_products/SampicTimeBin.h"
#include "analysis_pipeline/unpacker_core/data_products/ByteStream.h"
#include "analysis_pipeline/core/data/pipeline_data_product.h"
#include <spdlog/spdlog.h>
#include <cstring>
#include <memory>
ClassImp(ByteStreamToSampicTimeBinStage);
void ByteStreamToSampicTimeBinStage::OnInit() {
    ByteStreamProcessorStage::OnInit();
    output_product_name_ = parameters_.value("output_product_name", "SampicTimeBin");
}
void ByteStreamToSampicTimeBinStage::Process() {
    auto input_lock = getInputByteStreamLock();
    if (!input_lock) return;
    auto bytes = std::dynamic_pointer_cast<dataProducts::ByteStream>(input_lock->getSharedObject());
    using Record = dataProducts::SampicTimeBinRecord;
    if (!bytes || !bytes->data || bytes->size != sizeof(Record)) {
        spdlog::error("[{}] Invalid SB bank length (expected 32 bytes)", Name());
        return;
    }
    Record record{};
    std::memcpy(&record, bytes->data, sizeof(record));
    if (record.format_version != 1 || record.record_size != sizeof(Record)) {
        spdlog::error("[{}] Unsupported SB version/record size", Name());
        return;
    }
    auto result = std::make_unique<dataProducts::SampicTimeBin>();
    result->format_version = record.format_version;
    result->record_size = record.record_size;
    result->hit_count = record.hit_count;
    result->bin_index = record.bin_index;
    result->start_ns = record.start_ns;
    result->end_ns = record.end_ns;
    auto product = std::make_unique<PipelineDataProduct>();
    product->setName(output_product_name_);
    product->setObject(std::move(result));
    product->addTag("sampic_time_bin");
    product->addTag("built_by_byte_stream_to_sampic_time_bin_stage");
    getDataProductManager()->addOrUpdate(output_product_name_, std::move(product));
}
