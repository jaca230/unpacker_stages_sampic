#include "analysis_pipeline/unpacker_sampic/stages/byte_stream_to_sampic_event_stage.h"
#include "analysis_pipeline/unpacker_sampic/data_products/SampicEvent.h"
#include "analysis_pipeline/unpacker_core/data_products/ByteStream.h"
#include "analysis_pipeline/core/data/pipeline_data_product.h"
#include <spdlog/spdlog.h>
#include <cstring>
#include <memory>

ClassImp(ByteStreamToSampicEventStage);
ByteStreamToSampicEventStage::ByteStreamToSampicEventStage() = default;
void ByteStreamToSampicEventStage::OnInit() {
    ByteStreamProcessorStage::OnInit();
    output_product_name_ = parameters_.value("output_product_name", "SampicEvent");
}
void ByteStreamToSampicEventStage::Process() {
    auto input_lock = getInputByteStreamLock();
    if (!input_lock) return;
    auto bytes = std::dynamic_pointer_cast<dataProducts::ByteStream>(input_lock->getSharedObject());
    using Header = dataProducts::SampicHitBankHeader;
    using Record = dataProducts::SampicHitRecord;
    if (!bytes || !bytes->data || bytes->size < sizeof(Header)) {
        spdlog::error("[{}] SD bank is shorter than its header", Name());
        return;
    }
    Header header{};
    std::memcpy(&header, bytes->data, sizeof(header));
    if (std::memcmp(header.magic, "SDC1", 4) != 0 || header.version != 1 ||
        header.record_size != sizeof(Record)) {
        spdlog::error("[{}] Unsupported SD magic/version/record size", Name());
        return;
    }
    const size_t payload_size = bytes->size - sizeof(Header);
    if (payload_size % sizeof(Record) != 0 || payload_size / sizeof(Record) != header.hit_count) {
        spdlog::error("[{}] SD hit count does not match bank length", Name());
        return;
    }
    auto event = std::make_unique<dataProducts::SampicEvent>();
    event->hits.reserve(header.hit_count);
    for (uint32_t i = 0; i < header.hit_count; ++i) {
        Record record{};
        std::memcpy(&record, bytes->data + sizeof(Header) + size_t(i) * sizeof(Record), sizeof(record));
        if (record.waveform_size > dataProducts::MAX_SAMPIC_SAMPLES) {
            spdlog::error("[{}] Invalid SD waveform size {}", Name(), record.waveform_size);
            return;
        }
        dataProducts::SampicHitData hit;
        hit.fe_board_index = record.fe_board_index;
        hit.channel = record.channel;
        hit.hit_number = record.hit_number;
        hit.sampic_index = record.sampic_index;
        hit.channel_index = record.channel_index;
        hit.flags = record.flags;
        hit.inl_corrected = (record.flags & 1) != 0;
        hit.adc_corrected = (record.flags & 2) != 0;
        hit.residual_pedestal_corrected = (record.flags & 4) != 0;
        hit.waveform_clipped = (record.flags & 8) != 0;
        hit.waveform_nonfinite = (record.flags & 16) != 0;
        hit.raw_tot_clipped = (record.flags & 32) != 0;
        hit.cell_info = record.cell_info;
        hit.first_cell_physical_index = record.first_cell_physical_index;
        hit.encoded_waveform.assign(record.waveform, record.waveform + record.waveform_size);
        hit.corrected_waveform.reserve(record.waveform_size);
        for (auto sample : hit.encoded_waveform)
            hit.corrected_waveform.push_back(hit.adc_corrected ? sample / 10000.0f : float(sample));
        hit.raw_tot_value = record.raw_tot_value;
        hit.tot_value = record.tot_value;
        hit.first_cell_timestamp = record.first_cell_timestamp;
        event->hits.push_back(std::move(hit));
    }
    auto product = std::make_unique<PipelineDataProduct>();
    product->setName(output_product_name_);
    product->setObject(std::move(event));
    product->addTag("sampic_event");
    product->addTag("built_by_byte_stream_to_sampic_event_stage");
    getDataProductManager()->addOrUpdate(output_product_name_, std::move(product));
}
