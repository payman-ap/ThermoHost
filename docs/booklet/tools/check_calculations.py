"""Reproduce selected textbook examples with exact rational arithmetic.

All values beyond current ThermoHost constants are declared hypothetical
assumptions, not device measurements. Run directly for the calculation table.
"""

from fractions import Fraction
from math import ceil
from pathlib import Path
import re


ROOT = Path(__file__).resolve().parents[3]


def calculations():
    firmware = (ROOT / 'firmware/thermometer_usb.c').read_text(encoding='utf-8')
    assert re.search(r'#define\s+CONVERSION_INTERVAL_MS\s+250\b', firmware)
    assert 'spi_init(spi0, 1000000)' in firmware
    reads_per_second = Fraction(1000, 250)
    thermometer_traffic = reads_per_second * (20 + 32)
    payload_us = Fraction(52 * 8 * 1_000_000, 12_000_000)
    spi_us = Fraction(16 * 1_000_000, 1_000_000)
    modeled_read_ms = Fraction(250) + Fraction(spi_us, 1000) + 2
    assert reads_per_second == 4 and thermometer_traffic == 208
    assert payload_us == Fraction(104, 3) and modeled_read_ms == Fraction(31502, 125)

    channels, scans_per_second, bytes_per_sample = 8, 20_000, 3
    scans_per_block, header_bytes = 32, 32
    sample_rate = channels * scans_per_second * bytes_per_sample
    block_payload = channels * scans_per_block * bytes_per_sample
    block_bytes = block_payload + header_bytes
    blocks_per_second = Fraction(scans_per_second, scans_per_block)
    app_rate = block_bytes * blocks_per_second
    assert (sample_rate, block_payload, block_bytes, blocks_per_second, app_rate) == (
        480_000, 768, 800, 625, 500_000)
    assert Fraction(block_payload, block_bytes) == Fraction(24, 25)
    assert ceil(Fraction(block_bytes, 64)) == 13
    assert block_bytes % 64 == 32
    assert Fraction((scans_per_block - 1) * 1000, scans_per_second) == Fraction(31, 20)

    fs_bulk_ceiling = 19 * 64 * 1000
    assert fs_bulk_ceiling == 1_216_000
    assert Fraction(app_rate, 800_000) == Fraction(5, 8)
    assert 8 * 100_000 * 3 * 8 > 12_000_000
    gap_buffer = app_rate * Fraction(20, 1000)
    assert gap_buffer == 10_000
    buffer_seconds = Fraction(64 * 1024, app_rate)
    assert buffer_seconds == Fraction(2048, 15625)
    overflow_seconds = Fraction(64 * 1024, app_rate - 400_000)
    assert overflow_seconds == Fraction(2048, 3125)
    padded_block = 8 * 32 * 4 + 32
    assert padded_block * blocks_per_second == 660_000
    assert ceil(Fraction(padded_block, 64)) == 17

    stereo_rate = 2 * 48_000 * 3
    assert stereo_rate == 288_000 and stereo_rate / 1000 <= 1023
    assert 2 * 48_000 * 4 == 384_000
    assert 8 * 96_000 * 4 == 3_072_000
    assert 1024 * 8000 == 8_192_000
    drift_sample_frames = Fraction(48_000 * 100, 1_000_000)
    drift_bytes = drift_sample_frames * 6
    audio_buffer = stereo_rate * Fraction(10, 1000)
    assert drift_sample_frames == Fraction(24, 5)
    assert drift_bytes == Fraction(144, 5)
    assert audio_buffer == 2880 and audio_buffer / drift_bytes == 100
    assert audio_buffer / (drift_bytes / 2) == 200

    stream_rate, transfer_bytes = 8_000_000, 4096
    replenish_seconds = Fraction(2, 1000)
    depth = 1 + ceil(Fraction(stream_rate * replenish_seconds, transfer_bytes))
    coverage_ms = Fraction((depth - 1) * transfer_bytes * 1000, stream_rate)
    assert depth == 5 and coverage_ms == Fraction(256, 125)
    assert (depth - 2) * transfer_bytes < stream_rate * replenish_seconds
    return [
        ('Thermometer reads/s (ideal)', reads_per_second),
        ('Thermometer application B/s', thermometer_traffic),
        ('Payload-only serialization us', payload_us),
        ('Modeled thermometer read ms', modeled_read_ms),
        ('Packed DAQ application B/s', app_rate),
        ('Optimistic FS bulk ceiling B/s', fs_bulk_ceiling),
        ('20 ms pause: free buffer bytes', gap_buffer),
        ('64 KiB coverage seconds', buffer_seconds),
        ('100 kB/s mismatch: fill seconds', overflow_seconds),
        ('Stereo packed 24-bit B/s', stereo_rate),
        ('100 ppm mismatch B/s', drift_bytes),
        ('10 ms buffer: drift fill seconds', audio_buffer / drift_bytes),
        ('Posted buffer count (model)', depth),
        ('After-completion coverage ms', coverage_ms),
    ]


if __name__ == '__main__':
    for label, value in calculations():
        print(f'{label:42s} {float(value):14.6f}')
    print('Selected textbook calculations passed (models, not measurements).')
