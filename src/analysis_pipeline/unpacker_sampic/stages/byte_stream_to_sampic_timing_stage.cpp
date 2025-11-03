#include "analysis_pipeline/unpacker_sampic/stages/byte_stream_to_sampic_timing_stage.h"

#include "analysis_pipeline/unpacker_sampic/data_products/SampicTiming.h"
#include "analysis_pipeline/unpacker_core/data_products/ByteStream.h"
#include "analysis_pipeline/core/data/pipeline_data_product.h"

#include <spdlog/spdlog.h>
#include <memory>
#include <cstring>

ClassImp(ByteStreamToSampicTimingStage);

ByteStreamToSampicTimingStage::ByteStreamToSampicTimingStage() = default;

void ByteStreamToSampicTimingStage::OnInit() {
    ByteStreamProcessorStage::OnInit();

    output_product_name_ = parameters_.value("output_product_name", "SampicTiming");
    spdlog::debug("[{}] Initialized with output product name '{}'", Name(), output_product_name_);
}

void ByteStreamToSampicTimingStage::Process() {
    auto input_lock = getInputByteStreamLock();
    if (!input_lock) {
        spdlog::debug("[{}] Could not lock ByteStream '{}'", Name(), input_byte_stream_product_name_);
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

    // Create output timing data
    auto timing = std::make_unique<dataProducts::SampicTiming>();

    // Determine bank type based on size
    // AT banks: sizeof(SampicEventTimingRecord) = 48 bytes
    // AC banks: sizeof(SampicCollectorTimingRecord) = 28 bytes

    if (remaining == sizeof(dataProducts::SampicEventTimingRecord)) {
        // Parse AT bank (event timing)
        dataProducts::SampicEventTimingRecord record;
        std::memcpy(&record, ptr, sizeof(record));

        // Use the max values as representative event timing
        // (could also use sum/nparents for average)
        timing->event_prepare_time_us = static_cast<double>(record.sp_prepare_us_max);
        timing->event_read_time_us = static_cast<double>(record.sp_read_us_max);
        timing->event_decode_time_us = static_cast<double>(record.sp_decode_us_max);
        timing->event_total_time_us = static_cast<double>(record.sp_total_us_max);

        spdlog::debug("[{}] Parsed AT bank: {} hits, {} parents, total={} us",
                    Name(), record.nhits, record.nparents, record.sp_total_us_max);

    } else if (remaining == sizeof(dataProducts::SampicCollectorTimingRecord)) {
        // Parse AC bank (collector timing)
        dataProducts::SampicCollectorTimingRecord record;
        std::memcpy(&record, ptr, sizeof(record));

        timing->collector_wait_time_us = static_cast<double>(record.wait_us);
        timing->collector_group_time_us = static_cast<double>(record.group_build_us);
        timing->collector_finalize_time_us = static_cast<double>(record.finalize_us);
        timing->collector_total_time_us = static_cast<double>(record.total_us);
        // Note: bank_creation_time is not in the AC record, leave as 0

        spdlog::debug("[{}] Parsed AC bank: {} events, {} hits, total={} us",
                    Name(), record.n_events, record.total_hits, record.total_us);

    } else {
        spdlog::warn("[{}] Unexpected ByteStream size {} (expected {} for AT or {} for AC)",
                    Name(), remaining,
                    sizeof(dataProducts::SampicEventTimingRecord),
                    sizeof(dataProducts::SampicCollectorTimingRecord));
        return;
    }

    // Create data product
    auto product = std::make_unique<PipelineDataProduct>();
    product->setName(output_product_name_);
    product->setObject(std::move(timing));
    product->addTag("sampic_timing");
    product->addTag("built_by_byte_stream_to_sampic_timing_stage");

    getDataProductManager()->addOrUpdate(output_product_name_, std::move(product));
}
