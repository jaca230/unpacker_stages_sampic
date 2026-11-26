#pragma once
#include "analysis_pipeline/unpacker_sampic/data_products/SampicBuilderEvent.h"
#include "analysis_pipeline/unpacker_sampic/data_products/SampicBuilderDiagnostics.h"
#include <cstddef>
#include <utility>
namespace sampicBuilder {
dataProducts::SampicBuilderEvent decodeEvent(const unsigned char*, std::size_t);
dataProducts::SampicBuilderTimingSummary decodeSummary(const unsigned char*, std::size_t, unsigned short crate);
std::pair<std::uint64_t,std::vector<dataProducts::SampicBuilderCutSummary>> decodeCuts(const unsigned char*, std::size_t);
dataProducts::SampicBuilderCollectorCycle decodeCollector(const unsigned char*, std::size_t);
}
