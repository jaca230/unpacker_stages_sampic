# unpacker_stages_sampic

Byte-stream stages for the current compact SAMPIC frontend banks. Requires the
matching `unpacker_data_products_sampic` checkout.

- `ByteStreamToSampicEventStage`: SD magic `SDC1`, version 1, 154-byte hits.
- `ByteStreamToSampicHitMetadataStage`: SH version 2, 32-byte timing records.
- `ByteStreamToSampicReadoutTickStage`: SR version 1, 56-byte readout ticks.
- `ByteStreamToSampicTimeBinStage`: SB version 1, 32-byte fixed-time bins.
- Existing ST, SC, SG, and SV stages retain their current layouts.

Each stage takes `input_byte_stream_product_name` and `output_product_name`.
Absent banks produce no product. Empty versioned SD/SH banks produce empty
containers. Malformed lengths, unsupported versions/record sizes, bad SD magic,
and waveform lengths above 64 are diagnosed without publishing partial products.
Waveform scaling and quality flags follow the data-product README.

No historical SD/SH fallback is performed. The producer must use compact output
(`verbose_banks=false`). The app's `tests/verify_current_banks.py` compares all
records against complete real acquisitions; `tests/check_bank_edge_cases.py`
covers empty and metadata-only events, signed samples, flags, frontend suffixes,
and malformed banks through the ROOT output pipeline.
