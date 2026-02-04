from fastapi import FastAPI, Request
from fastapi.responses import HTMLResponse, JSONResponse
from fastapi.staticfiles import StaticFiles
from fastapi.templating import Jinja2Templates
import uvicorn
import asyncpg
import os
import asyncio
from typing import List, Optional
from pydantic import BaseModel

app = FastAPI()

# Use absolute paths relative to this file
BASE_DIR = os.path.dirname(os.path.abspath(__file__))
FRAMES_DIR = os.path.join(BASE_DIR, "../detected_frames")
HLS_DIR = os.path.join(BASE_DIR, "../hls_output")
TEMPLATE_DIR = os.path.join(BASE_DIR, "templates")

# Mount static files (images and HLS)
app.mount("/images", StaticFiles(directory=FRAMES_DIR), name="images")
app.mount("/hls", StaticFiles(directory=HLS_DIR), name="hls")

templates = Jinja2Templates(directory=TEMPLATE_DIR)

# Database Connection Pool
pool = None

@app.on_event("startup")
async def startup():
    global pool
    # Connect to PostgreSQL
    # Using defaults for localhost/dev. 
    # Production would use env vars: os.getenv("DB_HOST", "localhost")
    try:
        pool = await asyncpg.create_pool(
            user="admin", 
            password="password", 
            database="analytics_db", 
            host="localhost"
        )
        print("Connected to PostgreSQL")
    except Exception as e:
        print(f"Failed to connect to DB: {e}")

@app.on_event("shutdown")
async def shutdown():
    if pool:
        await pool.close()

@app.get("/", response_class=HTMLResponse)
async def read_root(request: Request):
    return templates.TemplateResponse("index.html", {"request": request})

@app.get("/detections")
async def get_detections():
    if not pool:
        return []
    async with pool.acquire() as connection:
        # Fetch last 20 detections
        rows = await connection.fetch("""
            SELECT device_name, class_name, confidence, timestamp, frame_path 
            FROM detections 
            ORDER BY id DESC 
            LIMIT 20
        """)
        return [dict(row) for row in rows]

if __name__ == "__main__":
    uvicorn.run(app, host="0.0.0.0", port=9090)
