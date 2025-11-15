import cv2
import numpy as np
import time
import serial
from PIL import Image
import common as cm  # module bạn có sẵn
import sys

# ------------------ CẤU HÌNH SERIAL ------------------
ser = serial.Serial('COM7', 115200, timeout=1.0)
ser.reset_input_buffer()

def send_signal(has_person):
    """Gửi tín hiệu qua Serial: '1' nếu có người, '0' nếu không có người."""
    if has_person:
        ser.write("1\n".encode('utf-8'))
        print(">> Có người!")
    else:
        ser.write("0\n".encode('utf-8'))
        print(">> Không có người.")

# ------------------ CẤU HÌNH MÔ HÌNH ------------------
model_dir = '../../all_models'
model = 'mobilenet_ssd_v2_coco_quant_postprocess.tflite'
lbl = 'coco_labels.txt'
threshold = 0.5  # độ tin cậy tối thiểu

# ------------------ KHỞI TẠO CAMERA ------------------
cap = cv2.VideoCapture(0)
if not cap.isOpened():
    print("Không mở được camera!")
    exit()

# ------------------ NẠP MÔ HÌNH ------------------
interpreter, labels = cm.load_model(model_dir, model, lbl, 0)

print("Bắt đầu nhận diện người... Nhấn 'q' để thoát.")
last_send_time = 0
interval = 1.0  # giãn cách gửi tín hiệu (giây)

while True:
    ret, frame = cap.read()
    if not ret:
        print("Không đọc được khung hình từ camera!")
        break

    # Chuyển đổi định dạng hình ảnh
    cv2_im_rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
    pil_im = Image.fromarray(cv2_im_rgb)

    # Inference
    cm.set_input(interpreter, pil_im)
    interpreter.invoke()
    objs = cm.get_output(interpreter, score_threshold=threshold, top_k=3)

    # Kiểm tra có người hay không
    has_person = any(labels.get(obj.id, obj.id) == "person" for obj in objs)

    # Gửi tín hiệu mỗi giây một lần để tránh spam
    if time.time() - last_send_time > interval:
        send_signal(has_person)
        last_send_time = time.time()

    # Hiển thị video (tùy chọn)
    for obj in objs:
        if labels.get(obj.id, obj.id) == "person":
            x0, y0, x1, y1 = list(obj.bbox)
            height, width, _ = frame.shape
            x0, y0, x1, y1 = int(x0*width), int(y0*height), int(x1*width), int(y1*height)
            cv2.rectangle(frame, (x0, y0), (x1, y1), (0, 255, 0), 2)
            cv2.putText(frame, "Person", (x0, y0-10),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 0), 2)

    cv2.imshow("Person Detection", frame)

    # Nhấn q để thoát
    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

cap.release()
cv2.destroyAllWindows()
ser.close()
print("Kết thúc chương trình.")
