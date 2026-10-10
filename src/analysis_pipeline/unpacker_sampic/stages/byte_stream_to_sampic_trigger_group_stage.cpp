#include "analysis_pipeline/unpacker_sampic/stages/byte_stream_to_sampic_trigger_group_stage.h"

#include "analysis_pipeline/core/data/pipeline_data_product.h"
#include "analysis_pipeline/unpacker_core/data_products/ByteStream.h"
#include "analysis_pipeline/unpacker_sampic/data_products/SampicTriggerGroup.h"

#include <spdlog/spdlog.h>

#include <cstring>
#include <memory>

ClassImp(ByteStreamToSampicTriggerGroupStage);

void ByteStreamToSampicTriggerGroupStage::OnInit() {
    ByteStreamProcessorStage::OnInit();
    output_product_name_ = parameters_.value("output_product_name", "SampicTriggerGroup");
}

void ByteStreamToSampicTriggerGroupStage::Process() {
    auto input_lock = getInputByteStreamLock();
    if (!input_lock) {
        return;
    }

    auto byte_stream = std::dynamic_pointer_cast<dataProducts::ByteStream>(
        input_lock->getSharedObject());
    if (!byte_stream || !byte_stream->data ||
        byte_stream->size != sizeof(dataProducts::SampicTriggerGroupRecord)) {
        spdlog::warn("[{}] SG bank expected {} bytes but got {}", Name(),
                     sizeof(dataProducts::SampicTriggerGroupRecord),
                     byte_stream ? byte_stream->size : 0);
        return;
    }

    dataProducts::SampicTriggerGroupRecord record{};
    std::memcpy(&record, byte_stream->data, sizeof(record));

    auto group = std::make_unique<dataProducts::SampicTriggerGroup>();
    group->fpga_trigger_id = record.fpga_trigger_id;
    group->external_trigger_id = record.external_trigger_id;
    group->trigger_index_in_sampic_event = record.trigger_index_in_sampic_event;
    group->assigned_hits = record.assigned_hits;
    group->ambiguous_hits = record.ambiguous_hits;
    group->trigger_timestamp_ns = record.trigger_timestamp_ns;
    group->hit_reference_timestamp_ns = record.hit_reference_timestamp_ns;

    auto product = std::make_unique<PipelineDataProduct>();
    product->setName(output_product_name_);
    product->setObject(std::move(group));
    product->addTag("sampic_trigger_group");
    product->addTag("built_by_byte_stream_to_sampic_trigger_group_stage");
    getDataProductManager()->addOrUpdate(output_product_name_, std::move(product));
}
