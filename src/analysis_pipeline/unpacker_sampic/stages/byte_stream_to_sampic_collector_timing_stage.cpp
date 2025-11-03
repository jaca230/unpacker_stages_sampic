#include "analysis_pipeline/unpacker_sampic/stages/byte_stream_to_sampic_collector_timing_stage.h"

#include "analysis_pipeline/unpacker_sampic/data_products/SampicCollectorTiming.h"
#include "analysis_pipeline/unpacker_core/data_products/ByteStream.h"
#include "analysis_pipeline/core/data/pipeline_data_product.h"

#include <spdlog/spdlog.h>
#include <memory>
#include <cstring>

ClassImp(ByteStreamToSampicCollectorTimingStage);

ByteStreamToSampicCollectorTimingStage::ByteStreamToSampicCollectorTimingStage() = default;

void ByteStreamToSampicCollectorTimingStage::OnInit() {
    ByteStreamProcessorStage::OnInit();

    output_product_name_ = parameters_.value("output_product_name", "SampicCollectorTiming");
}

void ByteStreamToSampicCollectorTimingStage::Process() {
    auto input_lock = getInputByteStreamLock();
    if (!input_lock) {
        return;
    }

    auto base_obj = input_lock->getSharedObject();
    auto byte_stream = std::dynamic_pointer_cast<dataProducts::ByteStream>(base_obj);

    if (!byte_stream || !byte_stream->data || byte_stream->size == 0) {
        spdlog::warn("[{}] Invalid or empty ByteStream", Name());
        return;
    }

    const uint8_t* ptr = byte_stream->data;
    size_t remaining = byte_stream->size;

    // Verify size matches expected structure
    if (remaining != sizeof(dataProducts::SampicCollectorTimingRecord)) {
        spdlog::warn("[{}] Expected collector timing bank size {} but got {}",
                    Name(), sizeof(dataProducts::SampicCollectorTimingRecord), remaining);
        return;
    }

    // Parse collector timing record
    dataProducts::SampicCollectorTimingRecord record;
    std::memcpy(&record, ptr, sizeof(record));

    // Create output data product and populate all fields
    auto timing = std::make_unique<dataProducts::SampicCollectorTiming>();

    // Collection cycle metadata
    timing->collector_timestamp_ns = record.collector_timestamp_ns;
    timing->n_events = record.n_events;
    timing->total_hits = record.total_hits;

    // Collector timing information
    timing->wait_us = record.wait_us;
    timing->group_build_us = record.group_build_us;
    timing->finalize_us = record.finalize_us;
    timing->total_us = record.total_us;

    // Create data product
    auto product = std::make_unique<PipelineDataProduct>();
    product->setName(output_product_name_);
    product->setObject(std::move(timing));
    product->addTag("sampic_collector_timing");
    product->addTag("built_by_byte_stream_to_sampic_collector_timing_stage");

    getDataProductManager()->addOrUpdate(output_product_name_, std::move(product));
}
