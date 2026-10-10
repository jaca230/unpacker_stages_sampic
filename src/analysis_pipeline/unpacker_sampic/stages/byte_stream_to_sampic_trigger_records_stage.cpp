#include "analysis_pipeline/unpacker_sampic/stages/byte_stream_to_sampic_trigger_records_stage.h"

#include "analysis_pipeline/core/data/pipeline_data_product.h"
#include "analysis_pipeline/unpacker_core/data_products/ByteStream.h"
#include "analysis_pipeline/unpacker_sampic/data_products/SampicTriggerRecords.h"

#include <spdlog/spdlog.h>

#include <cstring>
#include <limits>
#include <memory>

ClassImp(ByteStreamToSampicTriggerRecordsStage);

void ByteStreamToSampicTriggerRecordsStage::OnInit() {
    ByteStreamProcessorStage::OnInit();
    output_product_name_ = parameters_.value("output_product_name", "SampicTriggerRecords");
}

void ByteStreamToSampicTriggerRecordsStage::Process() {
    auto input_lock = getInputByteStreamLock();
    if (!input_lock) {
        return;
    }

    auto byte_stream = std::dynamic_pointer_cast<dataProducts::ByteStream>(
        input_lock->getSharedObject());
    if (!byte_stream || !byte_stream->data ||
        byte_stream->size < sizeof(dataProducts::SampicTriggerRecordBankHeader)) {
        spdlog::warn("[{}] Invalid or truncated SV bank", Name());
        return;
    }

    dataProducts::SampicTriggerRecordBankHeader header{};
    std::memcpy(&header, byte_stream->data, sizeof(header));
    if (header.count >
        (std::numeric_limits<size_t>::max() - sizeof(header)) / sizeof(dataProducts::SampicTriggerRecord)) {
        spdlog::warn("[{}] SV trigger count is too large: {}", Name(), header.count);
        return;
    }
    const size_t expected_size = sizeof(header) +
        static_cast<size_t>(header.count) * sizeof(dataProducts::SampicTriggerRecord);
    if (byte_stream->size != expected_size) {
        spdlog::warn("[{}] SV bank expected {} bytes but got {}", Name(), expected_size,
                     byte_stream->size);
        return;
    }

    auto records = std::make_unique<dataProducts::SampicTriggerRecords>();
    records->triggers.reserve(header.count);
    const uint8_t* ptr = byte_stream->data + sizeof(header);
    for (uint32_t index = 0; index < header.count; ++index) {
        dataProducts::SampicTriggerRecord record{};
        std::memcpy(&record, ptr, sizeof(record));
        ptr += sizeof(record);

        dataProducts::SampicTriggerRecordData trigger;
        trigger.fpga_trigger_id = record.fpga_trigger_id;
        trigger.external_trigger_id = record.external_trigger_id;
        trigger.spill_number = record.spill_number;
        trigger.raw_extra_word = record.raw_extra_word;
        trigger.trigger_timestamp_ns = record.trigger_timestamp_ns;
        records->triggers.push_back(std::move(trigger));
    }

    auto product = std::make_unique<PipelineDataProduct>();
    product->setName(output_product_name_);
    product->setObject(std::move(records));
    product->addTag("sampic_trigger_records");
    product->addTag("built_by_byte_stream_to_sampic_trigger_records_stage");
    getDataProductManager()->addOrUpdate(output_product_name_, std::move(product));
}
