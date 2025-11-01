#include "analysis_pipeline/unpacker_sampic/stages/byte_stream_to_sampic_event_stage.h"

#include "analysis_pipeline/unpacker_sampic/data_products/SampicEvent.h"
#include "analysis_pipeline/unpacker_sampic/data_products/SampicEventHeader.h"
#include "analysis_pipeline/unpacker_sampic/data_products/SampicPacket.h"
#include "analysis_pipeline/unpacker_sampic/data_products/SampicEventFooter.h"

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

    // Check space for header
    if (remaining < sizeof(dataProducts::SampicEventHeader)) {
        spdlog::error("[{}] Insufficient bytes for header (need {}, got {})",
                      Name(), sizeof(dataProducts::SampicEventHeader), remaining);
        return;
    }

    // --- Parse Header ---
    dataProducts::SampicEventHeader header;
    std::memcpy(&header, ptr, sizeof(header));
    ptr += sizeof(header);
    remaining -= sizeof(header);

    if (header.packet_size != sizeof(dataProducts::SampicPacket)) {
        spdlog::error("[{}] Packet size mismatch: header.packet_size={} != sizeof(SampicPacket)={}",
                      Name(), header.packet_size, sizeof(dataProducts::SampicPacket));
        return;
    }

    const size_t expected_size =
        sizeof(dataProducts::SampicEventHeader) +
        header.num_packets * sizeof(dataProducts::SampicPacket) +
        sizeof(dataProducts::SampicEventFooter);

    if (expected_size != byte_stream->size) {
        spdlog::error("[{}] ByteStream size mismatch:\n"
                      "  Header says: num_packets={}, packet_size={}\n"
                      "  Expected size: {}\n"
                      "  Actual ByteStream size: {}\n",
                      Name(), header.num_packets, header.packet_size,
                      expected_size, byte_stream->size);
        return;
    }

    // --- Parse Packets ---
    dataProducts::SampicPacketCollection packet_collection;
    for (uint16_t i = 0; i < header.num_packets; ++i) {
        if (remaining < sizeof(dataProducts::SampicPacket)) {
            spdlog::error("[{}] Truncated while reading packet {}/{}", Name(), i + 1, header.num_packets);
            return;
        }

        dataProducts::SampicPacket pkt;
        std::memcpy(&pkt, ptr, sizeof(pkt));
        ptr += sizeof(pkt);
        remaining -= sizeof(pkt);
        packet_collection.AddPacket(std::move(pkt));
    }

    // --- Parse Footer ---
    if (remaining < sizeof(dataProducts::SampicEventFooter)) {
        spdlog::error("[{}] Not enough bytes for footer (need {}, got {})",
                      Name(), sizeof(dataProducts::SampicEventFooter), remaining);
        return;
    }

    dataProducts::SampicEventFooter footer;
    std::memcpy(&footer, ptr, sizeof(footer));
    ptr += sizeof(footer);
    remaining -= sizeof(footer);

    // --- Assemble SampicEvent ---
    auto event = std::make_unique<dataProducts::SampicEvent>();
    event->header = header;
    event->packets = std::move(packet_collection);
    event->footer = footer;
    event->BuildWaveformsFromPackets();

    auto product = std::make_unique<PipelineDataProduct>();
    product->setName(output_product_name_);
    product->setObject(std::move(event));
    product->addTag("sampic_event");
    product->addTag("built_by_byte_stream_to_sampic_event_stage");

    getDataProductManager()->addOrUpdate(output_product_name_, std::move(product));

    spdlog::debug("[{}] Parsed SampicEvent from full ByteStream ({} bytes total)", Name(), byte_stream->size);
}

