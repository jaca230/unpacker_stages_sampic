#include "analysis_pipeline/unpacker_sampic/stages/byte_stream_to_sampic_event_stage.h"

#include "analysis_pipeline/unpacker_sampic/data_products/SampicEvent.h"
#include "analysis_pipeline/unpacker_sampic/data_products/SampicHit.h"

#include "analysis_pipeline/unpacker_core/data_products/ByteStream.h"
#include "analysis_pipeline/core/data/pipeline_data_product.h"

#include <spdlog/spdlog.h>
#include <memory>
#include <cstring>

ClassImp(ByteStreamToSampicEventStage);

ByteStreamToSampicEventStage::ByteStreamToSampicEventStage() = default;

void ByteStreamToSampicEventStage::OnInit() {
    ByteStreamProcessorStage::OnInit();

    output_product_name_ = parameters_.value("output_product_name", "SampicEvent");
    spdlog::debug("[{}] Initialized with output product name '{}'", Name(), output_product_name_);
}

void ByteStreamToSampicEventStage::Process() {
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

    // Create output event
    auto event = std::make_unique<dataProducts::SampicEvent>();

    // Parse all hits from the AD bank
    // Each hit consists of: Header + CorrectedDataSamples + Scalars
    size_t hit_count = 0;

    while (remaining > 0) {
        // Check if we have enough for header
        if (remaining < sizeof(dataProducts::SampicHitHeader)) {
            if (remaining > 0) {
                spdlog::warn("[{}] {} bytes remaining, not enough for another hit header",
                            Name(), remaining);
            }
            break;
        }

        // Parse header
        dataProducts::SampicHitHeader header;
        std::memcpy(&header, ptr, sizeof(header));
        ptr += sizeof(header);
        remaining -= sizeof(header);

        // Validate data_size
        if (header.data_size < 0 || header.data_size > dataProducts::MAX_SAMPIC_SAMPLES) {
            spdlog::error("[{}] Invalid data_size={} in hit {}", Name(), header.data_size, hit_count);
            break;
        }

        // Calculate waveform data size
        const size_t waveform_bytes = header.data_size * sizeof(float);

        // Check if we have enough for waveform
        if (remaining < waveform_bytes) {
            spdlog::error("[{}] Not enough bytes for waveform (need {}, got {})",
                        Name(), waveform_bytes, remaining);
            break;
        }

        // Parse waveform
        std::vector<float> waveform(header.data_size);
        std::memcpy(waveform.data(), ptr, waveform_bytes);
        ptr += waveform_bytes;
        remaining -= waveform_bytes;

        // Check if we have enough for scalars
        if (remaining < sizeof(dataProducts::SampicHitScalars)) {
            spdlog::error("[{}] Not enough bytes for scalars (need {}, got {})",
                        Name(), sizeof(dataProducts::SampicHitScalars), remaining);
            break;
        }

        // Parse scalars
        dataProducts::SampicHitScalars scalars;
        std::memcpy(&scalars, ptr, sizeof(scalars));
        ptr += sizeof(scalars);
        remaining -= sizeof(scalars);

        // Build SampicHitData
        dataProducts::SampicHitData hit_data;
        hit_data.fe_board_index = header.fe_board_index;
        hit_data.channel = header.channel;
        hit_data.hit_number = header.hit_number;
        hit_data.sampic_index = header.sampic_index;
        hit_data.channel_index = header.channel_index;
        hit_data.inl_corrected = (header.inl_corrected != 0);
        hit_data.adc_corrected = (header.adc_corrected != 0);
        hit_data.residual_pedestal_corrected = (header.residual_pedestal_corrected != 0);
        hit_data.cell_info = header.cell_info;
        hit_data.first_cell_physical_index = header.first_cell_physical_index;
        hit_data.corrected_waveform = std::move(waveform);
        hit_data.raw_tot_value = scalars.raw_tot_value;
        hit_data.tot_value = scalars.tot_value;
        hit_data.amplitude = scalars.amplitude;
        hit_data.baseline = scalars.baseline;
        hit_data.peak = scalars.peak;
        hit_data.time_index = scalars.time_index;
        hit_data.time_instant = scalars.time_instant;
        hit_data.time_amplitude = scalars.time_amplitude;
        hit_data.first_cell_timestamp = scalars.first_cell_timestamp;

        event->hits.push_back(std::move(hit_data));
        ++hit_count;
    }

    spdlog::debug("[{}] Parsed {} hits from AD bank ({} bytes total, {} bytes remaining)",
                Name(), hit_count, byte_stream->size, remaining);

    // Create data product
    auto product = std::make_unique<PipelineDataProduct>();
    product->setName(output_product_name_);
    product->setObject(std::move(event));
    product->addTag("sampic_event");
    product->addTag("built_by_byte_stream_to_sampic_event_stage");

    getDataProductManager()->addOrUpdate(output_product_name_, std::move(product));
}

