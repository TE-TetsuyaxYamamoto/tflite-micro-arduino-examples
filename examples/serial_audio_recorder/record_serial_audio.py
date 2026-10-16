"""Record 16 kHz mono PCM from the serial_audio_recorder Arduino example."""

import argparse
import struct
import wave
from pathlib import Path

import serial


SAMPLE_RATE = 16000
SAMPLE_WIDTH = 2
SECONDS_PER_FILE = 1
MAX_PACKET_SAMPLES = 512
MAGIC = b"AUD0"
SECOND_BYTES = SAMPLE_RATE * SAMPLE_WIDTH * SECONDS_PER_FILE


def read_exact(port, size):
    data = bytearray()
    while len(data) < size:
        chunk = port.read(size - len(data))
        if chunk:
            data.extend(chunk)
    return bytes(data)


def read_packet(port):
    sync = bytearray()
    while True:
        byte = port.read(1)
        if not byte:
            continue
        sync.extend(byte)
        if len(sync) > len(MAGIC):
            del sync[0]
        if sync == MAGIC:
            break

    sequence, sample_count = struct.unpack("<HH", read_exact(port, 4))
    if not 0 < sample_count <= MAX_PACKET_SAMPLES:
        return None
    return sequence, read_exact(port, sample_count * SAMPLE_WIDTH)


def write_wav(path, pcm_data):
    with wave.open(str(path), "wb") as wav_file:
        wav_file.setnchannels(1)
        wav_file.setsampwidth(SAMPLE_WIDTH)
        wav_file.setframerate(SAMPLE_RATE)
        wav_file.writeframes(pcm_data)


def next_output_path(output_dir, index):
    while True:
        path = output_dir / f"audio_{index:06d}.wav"
        if not path.exists():
            return path, index + 1
        index += 1


def main():
    parser = argparse.ArgumentParser(
        description="Save serial audio as consecutive one-second WAV files."
    )
    parser.add_argument("--port", required=True, help="Serial port, e.g. COM5")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--output-dir", type=Path, default=Path("recordings"))
    args = parser.parse_args()
    args.output_dir.mkdir(parents=True, exist_ok=True)

    pcm_data = bytearray()
    expected_sequence = None
    file_index = 1

    print(f"Recording from {args.port}; press Ctrl+C to stop.")
    try:
        with serial.Serial(args.port, args.baud, timeout=1) as port:
            while True:
                packet = read_packet(port)
                if packet is None:
                    continue

                sequence, samples = packet
                if expected_sequence is not None and sequence != expected_sequence:
                    missing = (sequence - expected_sequence) & 0xffff
                    if missing < 0x8000:
                        print(f"Warning: {missing} audio block(s) were dropped.")
                expected_sequence = (sequence + 1) & 0xffff
                pcm_data.extend(samples)

                while len(pcm_data) >= SECOND_BYTES:
                    path, file_index = next_output_path(args.output_dir, file_index)
                    write_wav(path, pcm_data[:SECOND_BYTES])
                    del pcm_data[:SECOND_BYTES]
                    print(f"Saved {path}")
    except KeyboardInterrupt:
        print("Stopped. Any incomplete final second was discarded.")


if __name__ == "__main__":
    main()