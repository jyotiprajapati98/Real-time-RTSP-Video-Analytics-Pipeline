import cv2
import numpy as np

width, height = 640, 480
fps = 30
duration = 10
out_file = 'test.mp4'

fourcc = cv2.VideoWriter_fourcc(*'mp4v')
out = cv2.VideoWriter(out_file, fourcc, fps, (width, height))

for i in range(fps * duration):
    frame = np.zeros((height, width, 3), dtype=np.uint8)
    
    # Moving circle
    x = int(width/2 + 100 * np.sin(2 * np.pi * i / fps))
    y = int(height/2 + 100 * np.cos(2 * np.pi * i / fps))
    
    cv2.circle(frame, (x, y), 50, (0, 0, 255), -1)
    
    # Add text
    cv2.putText(frame, f'Frame: {i}', (50, 50), cv2.FONT_HERSHEY_SIMPLEX, 1, (255, 255, 255), 2)
    
    out.write(frame)

out.release()
print(f"Generated {out_file}")
