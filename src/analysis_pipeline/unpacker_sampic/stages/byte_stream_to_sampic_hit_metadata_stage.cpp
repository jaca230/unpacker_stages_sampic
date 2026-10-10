#include "analysis_pipeline/unpacker_sampic/stages/byte_stream_to_sampic_hit_metadata_stage.h"

#include "analysis_pipeline/core/data/pipeline_data_product.h"
#include "analysis_pipeline/unpacker_core/data_products/ByteStream.h"
#include "analysis_pipeline/unpacker_sampic/data_products/SampicHitMetadata.h"

#include <spdlog/spdlog.h>

#include <cstring>
#include <limits>
#include <memory>

ClassImp(ByteStreamToSampicHitMetadataStage);

void ByteStreamToSampicHitMetadataStage::OnInit() {
    ByteStreamProcessorStage::OnInit();
    output_product_name_ = parameters_.value("output_product_name", "SampicHitMetadata");
}

void ByteStreamToSampicHitMetadataStage::Process() {
    auto input_lock = getInputByteStreamLock();
    if (!input_lock) {
        return;
    }

    auto byte_stream = std::dynamic_pointer_cast<dataProducts::ByteStream>(
        input_lock->getSharedObject());
    if (!byte_stream || !byte_stream->data) {
        spdlog::warn("[{}] Invalid ByteStream", Name());
        return;
    }

    const size_t bank_size = byte_stream->size;
    if (bank_size < sizeof(dataProducts::SampicHitMetadataBankHeader)) {
        spdlog::warn("[{}] SH bank is shorter than its {}-byte header", Name(),
                     sizeof(dataProducts::SampicHitMetadataBankHeader));
        return;
    }

    dataProducts::SampicHitMetadataBankHeader header{};
    std::memcpy(&header, byte_stream->data, sizeof(header));
    if (header.version != 1 || header.record_size != sizeof(dataProducts::SampicHitMetadataRecord)) {
        spdlog::warn("[{}] Unsupported SH format version/record size {}/{}", Name(),
                     header.version, header.record_size);
        return;
    }
    if (header.hit_count >
        (std::numeric_limits<size_t>::max() - sizeof(header)) / sizeof(dataProducts::SampicHitMetadataRecord)) {
        spdlog::warn("[{}] SH hit count is too large: {}", Name(), header.hit_count);
        return;
    }

    const size_t expected_size = sizeof(header) +
        static_cast<size_t>(header.hit_count) * sizeof(dataProducts::SampicHitMetadataRecord);
    if (bank_size != expected_size) {
        spdlog::warn("[{}] SH bank expected {} bytes but got {}", Name(), expected_size, bank_size);
        return;
    }

    auto metadata = std::make_unique<dataProducts::SampicHitMetadata>();
    metadata->hits.reserve(header.hit_count);
    const uint8_t* ptr = byte_stream->data + sizeof(header);
    for (uint32_t index = 0; index < header.hit_count; ++index) {
        dataProducts::SampicHitMetadataRecord record{};
        std::memcpy(&record, ptr, sizeof(record));
        ptr += sizeof(record);

        dataProducts::SampicHitMetadataData hit;
        hit.sampic_data_header = record.sampic_data_header;
        hit.first_trigger_position_cell = record.first_trigger_position_cell;
        hit.trigger_position_cell = record.trigger_position_cell;
        hit.physical_cell0_timestamp_ns = record.physical_cell0_timestamp_ns;
        hit.sampic_timestamp_a = record.sampic_timestamp_a;
        hit.sampic_timestamp_b = record.sampic_timestamp_b;
        hit.fpga_timestamp = record.fpga_timestamp;
        hit.adc_counter_latched_at_end_of_conversion =
            record.adc_counter_latched_at_end_of_conversion;
        hit.start_of_adc_ramp = record.start_of_adc_ramp;
        hit.trigger_position_mask = record.trigger_position_mask;
        hit.time_physical_index = record.time_physical_index;
        metadata->hits.push_back(std::move(hit));
    }

    auto product = std::make_unique<PipelineDataProduct>();
    product->setName(output_product_name_);
    product->setObject(std::move(metadata));
    product->addTag("sampic_hit_metadata");
    product->addTag("built_by_byte_stream_to_sampic_hit_metadata_stage");
    getDataProductManager()->addOrUpdate(output_product_name_, std::move(product));
}
