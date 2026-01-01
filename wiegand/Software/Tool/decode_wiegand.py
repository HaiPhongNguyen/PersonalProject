import csv
import sys
from pathlib import Path

# ================= CẤU HÌNH =====================

# Tên cột trong file CSV (đúng như Saleae export)
TIME_COL = "Time [s]"
D0_COL   = "Channel 5"   # D0
D1_COL   = "Channel 7"   # D1

# Ngưỡng tách frame: nếu khoảng cách giữa 2 bit > 5 ms => frame mới
FRAME_GAP_S = 0.005

# ================================================

def read_events(csv_path):
    """
    Đọc CSV, trả về list các sự kiện bit: (time_s, bit_value)
    bit_value = 0 nếu xung trên D0, 1 nếu xung trên D1
    """
    events = []
    prev_d0 = prev_d1 = None

    with open(csv_path, newline="") as f:
        reader = csv.DictReader(f)
        for row in reader:
            # Bỏ dòng time = 0 (header/dummy) nếu có
            try:
                t = float(row[TIME_COL])
            except ValueError:
                continue

            d0 = int(row[D0_COL])
            d1 = int(row[D1_COL])

            # Khởi tạo giá trị trước
            if prev_d0 is None:
                prev_d0, prev_d1 = d0, d1
                continue

            # Phát hiện cạnh xuống 1 -> 0
            if prev_d0 == 1 and d0 == 0:
                events.append((t, 0))  # bit 0
            if prev_d1 == 1 and d1 == 0:
                events.append((t, 1))  # bit 1

            prev_d0, prev_d1 = d0, d1

    return events


def split_frames(events, gap_s=FRAME_GAP_S):
    """
    Tách các sự kiện thành từng frame dựa trên khoảng trống lớn giữa các bit.
    Trả về list[list[(time, bit)]]
    """
    frames = []
    cur = []
    last_t = None

    for t, b in events:
        if last_t is not None and (t - last_t) > gap_s and cur:
            frames.append(cur)
            cur = []
        cur.append((t, b))
        last_t = t

    if cur:
        frames.append(cur)

    return frames


def bits_to_int(bits):
    """Chuyển list bit [b0,b1,...] thành số nguyên (MSB trước)."""
    val = 0
    for b in bits:
        val = (val << 1) | b
    return val


def decode_frame_bits(bits):
    """
    Nhận list bit, trả về dict thông tin giải mã (hoặc None nếu không 26/34).
    """
    n = len(bits)
    info = {
        "bit_count": n,
        "raw_bits": "".join(str(b) for b in bits),
        "facility": None,
        "card": None,
        "type": None,
    }

    if n == 26:
        # [P_even][8 fac][16 card][P_odd]
        fac_bits = bits[1:9]
        card_bits = bits[9:25]
        info["facility"] = bits_to_int(fac_bits)
        info["card"] = bits_to_int(card_bits)
        info["type"] = "Wiegand-26"

    elif n == 34:
        # [P_even][16 fac][16 card][P_odd]
        fac_bits = bits[1:17]
        card_bits = bits[17:33]
        info["facility"] = bits_to_int(fac_bits)
        info["card"] = bits_to_int(card_bits)
        info["type"] = "Wiegand-34"

    return info


def main(csv_path):
    events = read_events(csv_path)
    if not events:
        print("Không tìm thấy cạnh xuống nào (events rỗng).")
        return

    frames = split_frames(events)
    print(f"Found {len(frames)} frame(s)")

    for idx, frame in enumerate(frames, 1):
        bits = [b for _, b in frame]
        info = decode_frame_bits(bits)

        print(f"\nFrame {idx}: {len(bits)} bits")
        print(" Raw bits:", "".join(str(b) for b in bits))

        if info["type"] is None:
            print(" (Không phải 26 hoặc 34 bit chuẩn, bỏ qua decode FAC/CARD)")
            continue

        print(f" Type    : {info['type']}")
        print(f" Facility: {info['facility']} (0x{info['facility']:X})")
        print(f" Card    : {info['card']} (0x{info['card']:X})")


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python decode_wiegand.py <file.csv>")
        print("Không truyền tham số => dùng mặc định 'wiegand_capture.csv'")
        csv_file = "wiegand_capture.csv"
    else:
        csv_file = sys.argv[1]

    if not Path(csv_file).is_file():
        print(f"Không tìm thấy file: {csv_file}")
        sys.exit(1)

    main(csv_file)
