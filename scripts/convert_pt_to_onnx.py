from ultralytics import YOLO
import sys

def convert_model(pt_path):
    print(f"Loading model: {pt_path}")
    model = YOLO(pt_path)
    print("Exporting to ONNX...")
    path = model.export(format="onnx")
    print(f"Model exported to: {path}")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python3 convert_pt_to_onnx.py <model.pt>")
        sys.exit(1)
    
    convert_model(sys.argv[1])
