#include "analysis_pipeline/unpacker_sampic/stages/byte_stream_to_sampic_time_stage.h"

#include "analysis_pipeline/unpacker_sampic/data_products/SampicTime.h"
#include "analysis_pipeline/unpacker_core/data_products/ByteStream.h"
#include "analysis_pipeline/core/data/pipeline_data_product.h"

#include <spdlog/spdlog.h>
#include <memory>
#include <cstring>

ClassImp(ByteStreamToSampicTimeStage);

ByteStreamToSampicTimeStage::ByteStreamToSampicTimeStage() = default;

void ByteStreamToSampicTimeStage::OnInit() {
    ByteStreamProcessorStage::OnInit();
    output_product_name_ = parameters_.value("output_product_name", "SampicTime");
    spdlog::debug("[{}] Initialized with output product name '{}'", Name(), output_product_name_);
}

void ByteStreamToSampicTimeStage::Process() {
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

    if (byte_stream->size != sizeof(dataProducts::SampicTimeData)) {
        spdlog::error("[{}] ByteStream size mismatch: expected {} bytes for SampicTimeData, got {}",
                      Name(), sizeof(dataProducts::SampicTimeData), byte_stream->size);
        return;
    }

    dataProducts::SampicTimeData time_data;
    std::memcpy(&time_data, byte_stream->data, sizeof(dataProducts::SampicTimeData));

    auto sampic_time = std::make_unique<dataProducts::SampicTime>();
    sampic_time->time = time_data;

    auto product = std::make_unique<PipelineDataProduct>();
    product->setName(output_product_name_);
    product->setObject(std::move(sampic_time));
    product->addTag("sampic_time");
    product->addTag("built_by_byte_stream_to_sampic_time_stage");

    getDataProductManager()->addOrUpdate(output_product_name_, std::move(product));

    spdlog::debug("[{}] Parsed SampicTime object from {} bytes", Name(), byte_stream->size);
}

