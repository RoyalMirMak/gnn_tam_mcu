import serial
import time
import numpy as np
import sys
from tqdm import tqdm
from sklearn.metrics import (accuracy_score, precision_recall_fscore_support,
                             confusion_matrix, classification_report)

PORT = "COM8"
BAUD = 921600
DATA_FILE = "./data/test_w100_all.npz"
TIMEOUT = 15.0
NUM_SAMPLES = None
BYTE_SIZE = 20800

def print_metrics(y_true, y_pred):
    print("\nClassification Report:")
    print(classification_report(y_true, y_pred, zero_division=0))

    cm = confusion_matrix(y_true, y_pred)
    print("Confusion Matrix:")
    print(cm)

    acc = accuracy_score(y_true, y_pred)
    print(f"Accuracy: {acc*100:.2f}%)")

def main():
    print(f"Loading {DATA_FILE}...")
    data = np.load(DATA_FILE)

    inputs = data['inputs']
    labels = data['labels']
    total_available = len(inputs)
    print(f"Loaded {total_available} samples.")

    if NUM_SAMPLES is None:
        num_to_use = total_available
    else:
        num_to_use = min(NUM_SAMPLES, total_available)
        
    print(f"Benchmarking {num_to_use} samples at {BAUD} baud.")

    ser = serial.Serial(PORT, BAUD, timeout=TIMEOUT)
    time.sleep(2.0)
    ser.reset_input_buffer()
    ser.reset_output_buffer()

    true_labels = []
    predictions = []
    times_us = []

    for i in tqdm(range(num_to_use), desc="Benchmarking", unit="sample"):
        input_data = inputs[i]
        true_label = labels[i]
        input_bytes = input_data.astype('<f4').tobytes()

        ser.reset_input_buffer()
        ser.write(b'S' + input_bytes)
        ser.flush()

        result_line = None
        start_wait = time.perf_counter()
        
        while (time.perf_counter() - start_wait) < TIMEOUT:
            raw_line = ser.readline()
            line = raw_line.decode('utf-8', errors='ignore').strip()

            if not line:
                continue
            
            if line.startswith("RESULT"):
                result_line = line
                break
            elif line.startswith("ERR"):
                tqdm.write(f"STM32 Error: {line}")

        if result_line is None:
            tqdm.write(f"Sample {i}: Timeout waiting for response.")
            continue

        parts = result_line.split(':')
        pred_class = int(parts[1])
        conf = int(parts[2])
        t_us = int(parts[3])
        
        true_labels.append(true_label)
        predictions.append(pred_class)
        times_us.append(t_us)

    ser.close()

    true_labels = np.array(true_labels)
    predictions = np.array(predictions)

    times_us = np.array(times_us)
    times_ms = times_us / 1000.0
    print(f"Average inference time: {np.mean(times_ms)} ms")

    print_metrics(true_labels, predictions)

if __name__ == "__main__":
    main()