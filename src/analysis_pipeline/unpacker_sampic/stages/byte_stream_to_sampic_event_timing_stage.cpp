#include "analysis_pipeline/unpacker_sampic/stages/byte_stream_to_sampic_event_timing_stage.h"

#include "analysis_pipeline/unpacker_sampic/data_products/SampicEventTiming.h"
#include "analysis_pipeline/unpacker_core/data_products/ByteStream.h"
#include "analysis_pipeline/core/data/pipeline_data_product.h"

#include <spdlog/spdlog.h>
#include <memory>
#include <cstring>

ClassImp(ByteStreamToSampicEventTimingStage);

ByteStreamToSampicEventTimingStage::ByteStreamToSampicEventTimingStage() = default;

void ByteStreamToSampicEventTimingStage::OnInit() {
    ByteStreamProcessorStage::OnInit();

    output_product_name_ = parameters_.value("output_product_name", "SampicEventTiming");
}

void ByteStreamToSampicEventTimingStage::Process() {
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
    if (remaining != sizeof(dataProducts::SampicEventTimingRecord)) {
        spdlog::warn("[{}] Expected event timing bank size {} but got {}",
                    Name(), sizeof(dataProducts::SampicEventTimingRecord), remaining);
        return;
    }

    // Parse event timing record
    dataProducts::SampicEventTimingRecord record;
    std::memcpy(&record, ptr, sizeof(record));

    // Create output data product and populate all fields
    auto timing = std::make_unique<dataProducts::SampicEventTiming>();

    // Frontend event metadata
    timing->fe_timestamp_ns = record.fe_timestamp_ns;
    timing->nhits = record.nhits;
    timing->nparents = record.nparents;

    // SAMPIC timing sums
    timing->sp_prepare_us_sum = record.sp_prepare_us_sum;
    timing->sp_read_us_sum = record.sp_read_us_sum;
    timing->sp_decode_us_sum = record.sp_decode_us_sum;
    timing->sp_total_us_sum = record.sp_total_us_sum;

    // SAMPIC timing maxima
    timing->sp_prepare_us_max = record.sp_prepare_us_max;
    timing->sp_read_us_max = record.sp_read_us_max;
    timing->sp_decode_us_max = record.sp_decode_us_max;
    timing->sp_total_us_max = record.sp_total_us_max;

    // Acquisition retry stats
    timing->sp_acq_retry_max = record.sp_acq_retry_max;
    timing->sp_acq_retry_sum = record.sp_acq_retry_sum;

    // Create data product
    auto product = std::make_unique<PipelineDataProduct>();
    product->setName(output_product_name_);
    product->setObject(std::move(timing));
    product->addTag("sampic_event_timing");
    product->addTag("built_by_byte_stream_to_sampic_event_timing_stage");

    getDataProductManager()->addOrUpdate(output_product_name_, std::move(product));
}
