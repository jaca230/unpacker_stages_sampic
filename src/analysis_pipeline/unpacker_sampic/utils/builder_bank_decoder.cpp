#include "analysis_pipeline/unpacker_sampic/utils/builder_bank_decoder.h"
#include <stdexcept>
#include <cstring>
#include <cmath>
#include <set>
#include <tuple>
namespace sampicBuilder {
namespace {
class Reader {
public:
    Reader(const unsigned char* p, std::size_t size): p_(p), remaining_(size) {
        if (!p && size) throw std::runtime_error("null builder bank data");
    }
    std::uint64_t read(unsigned width) {
        if (remaining_ < width) throw std::runtime_error("truncated builder bank");
        std::uint64_t result = 0;
        for (unsigned i=0; i<width; ++i) result |= std::uint64_t(p_[i]) << (8*i);
        p_ += width; remaining_ -= width; return result;
    }
    std::int64_t signedValue(unsigned width) {
        const auto value=read(width);
        const auto sign=std::uint64_t{1}<<(width*8-1);
        return value<sign ? static_cast<std::int64_t>(value) :
            static_cast<std::int64_t>(value)-static_cast<std::int64_t>(sign*2);
    }
    float real32() {
        const std::uint32_t bits=read(4);
        float value; std::memcpy(&value,&bits,sizeof(value)); return value;
    }
    double real64() {
        const std::uint64_t bits=read(8);
        double value; std::memcpy(&value,&bits,sizeof(value)); return value;
    }
    void timingHeader(const char* magic) {
        if (remaining_ != 128) throw std::runtime_error("invalid builder timing bank length");
        if (std::memcmp(p_,magic,4)) throw std::runtime_error("invalid builder timing magic");
        read(4);
        if (read(2)!=1 || read(2)!=128) throw std::runtime_error("unsupported builder timing version/size");
    }
    std::size_t remaining() const { return remaining_; }
private:
    const unsigned char* p_;
    std::size_t remaining_;
};
}
dataProducts::SampicBuilderEvent decodeEvent(const unsigned char* p, std::size_t size) {
    Reader r(p,size);
    if (r.read(4)!=0x31424553 || r.read(2)!=4 || r.read(2)!=0)
        throw std::runtime_error("unsupported BD magic/version/reserved field");
    const auto count=r.read(4);
    dataProducts::SampicBuilderEvent event;
    event.start_ns=r.read(8); event.end_ns=r.read(8);
    if (event.start_ns>event.end_ns || r.remaining()%188 || count!=r.remaining()/188)
        throw std::runtime_error("invalid BD time interval or waveform count");
    event.hits.reserve(count);
    for (std::uint64_t i=0; i<count; ++i) {
        dataProducts::SampicBuilderHit hit;
        hit.crate=r.read(2);
        auto& d=hit.data;
        d.fe_board_index=r.read(1); d.channel=r.read(1);
        d.hit_number=r.signedValue(4);
        d.sampic_index=r.read(1); d.channel_index=r.read(1);
        const auto samples=r.read(1); d.flags=r.read(1);
        d.cell_info=r.read(1); d.first_cell_physical_index=r.read(1);
        if (d.fe_board_index>3 || d.channel>63 || d.sampic_index>3 || d.channel_index>15 ||
            samples>64 || d.cell_info>63 || d.first_cell_physical_index>63 || (d.flags & ~63))
            throw std::runtime_error("invalid BD compact SD hit fields");
        d.inl_corrected=d.flags & 1; d.adc_corrected=d.flags & 2;
        d.residual_pedestal_corrected=d.flags & 4;
        d.waveform_clipped=d.flags & 8; d.waveform_nonfinite=d.flags & 16; d.raw_tot_clipped=d.flags & 32;
        d.encoded_waveform.reserve(samples); d.corrected_waveform.reserve(samples);
        for (unsigned j=0; j<64; ++j) {
            const auto sample=r.signedValue(2);
            if (j<samples) {
                d.encoded_waveform.push_back(sample);
                d.corrected_waveform.push_back(d.adc_corrected ? sample/10000.0f : float(sample));
            }
        }
        d.raw_tot_value=r.read(2); d.tot_value=r.real32(); d.first_cell_timestamp=r.real64();
        if (!std::isfinite(d.first_cell_timestamp)) throw std::runtime_error("nonfinite BD timestamp");
        auto& m=hit.metadata;
        m.physical_cell0_timestamp_ns=r.real64();
        m.sampic_timestamp_a=r.signedValue(4); m.sampic_timestamp_b=r.signedValue(4);
        m.fpga_timestamp=r.read(8); m.trigger_position_mask=r.read(8);
        event.hits.push_back(std::move(hit));
    }
    if (r.remaining()) throw std::runtime_error("trailing BD bank bytes");
    return event;
}

std::pair<std::uint64_t,std::vector<dataProducts::SampicBuilderCutSummary>> decodeCuts(const unsigned char* p,std::size_t size) {
    Reader r(p,size);
    if (r.read(4)!=0x31535342 || r.read(2)!=1 || r.read(2)!=72)
        throw std::runtime_error("unsupported BS magic/version/record size");
    const auto cycle=r.read(8); const auto count=r.read(4);
    if (r.read(4)!=0 || r.remaining()%72 || count!=r.remaining()/72)
        throw std::runtime_error("invalid BS reserved/count/length");
    std::vector<dataProducts::SampicBuilderCutSummary> cuts;
    std::set<std::string> ids;
    for (std::uint64_t i=0;i<count;++i) {
        dataProducts::SampicBuilderCutSummary c;
        bool terminated=false;
        for (int j=0;j<32;++j) {
            const char ch=r.read(1);
            if (terminated && ch) throw std::runtime_error("nonzero BS name padding");
            if (!ch) terminated=true;
            else if (!((ch>='a' && ch<='z') || (ch>='0' && ch<='9') || ch=='_'))
                throw std::runtime_error("invalid BS cut name");
            else c.id+=ch;
        }
        const auto enabled=r.read(4); c.enabled=enabled!=0; c.priority=r.signedValue(4);
        c.evaluated_events=r.read(8); c.rejected_events=r.read(8);
        c.evaluated_hits=r.read(8); c.rejected_hits=r.read(8);
        if (!terminated || c.id.empty() || !ids.insert(c.id).second || enabled>1 ||
            c.rejected_events>c.evaluated_events || c.rejected_hits>c.evaluated_hits ||
            (!c.enabled && (c.evaluated_events || c.rejected_events || c.evaluated_hits || c.rejected_hits)) ||
            (!c.evaluated_events && c.evaluated_hits) || (!c.rejected_events && c.rejected_hits))
            throw std::runtime_error("invalid BS cut counters/configuration");
        if (!cuts.empty() && std::tie(c.priority,c.id)<std::tie(cuts.back().priority,cuts.back().id))
            throw std::runtime_error("BS cuts are not priority ordered");
        cuts.push_back(std::move(c));
    }
    return {cycle,std::move(cuts)};
}
dataProducts::SampicBuilderTimingSummary decodeSummary(const unsigned char* p, std::size_t size, unsigned short crate) {
    Reader r(p,size); r.timingHeader("BTS1");
    dataProducts::SampicBuilderTimingSummary result; result.crate=crate;
    result.cycle_id=r.read(8);
    result.read_count=r.read(8);
    result.hit_count=r.read(8);
    result.prepare_us_sum=r.read(8);
    result.read_us_sum=r.read(8);
    result.decode_us_sum=r.read(8);
    result.total_us_sum=r.read(8);
    result.prepare_us_max=r.read(8);
    result.read_us_max=r.read(8);
    result.decode_us_max=r.read(8);
    result.total_us_max=r.read(8);
    result.retries_sum=r.read(8);
    result.retries_max=r.read(8);
    result.first_acquisition_id=r.read(8);
    result.last_acquisition_id=r.read(8);
    if (!result.read_count || result.first_acquisition_id>result.last_acquisition_id ||
        result.last_acquisition_id-result.first_acquisition_id != result.read_count-1)
        throw std::runtime_error("invalid BT acquisition ID range");
    return result;
}
dataProducts::SampicBuilderCollectorCycle decodeCollector(const unsigned char* p, std::size_t size) {
    Reader r(p,size); r.timingHeader("BCT1");
    dataProducts::SampicBuilderCollectorCycle result;
    result.cycle_id=r.read(8);
    result.start_ns=r.read(8);
    result.input_events=r.read(8);
    result.input_hits=r.read(8);
    result.output_events=r.read(8);
    result.output_hits=r.read(8);
    result.selected_events=r.read(8);
    result.rejected_events=r.read(8);
    result.input_read_ns=r.read(8);
    result.group_build_ns=r.read(8);
    result.cuts_ns=r.read(8);
    result.finalize_ns=r.read(8);
    result.output_wait_ns=r.read(8);
    result.total_ns=r.read(8);
    result.flags=r.read(8);
    if (result.flags & ~std::uint64_t(1)) throw std::runtime_error("unsupported BC flags");
    return result;
}
} // namespace sampicBuilder
