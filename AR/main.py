from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware
 
app = FastAPI()
 
origins = [
    "http://localhost:3000",
    "http://127.0.0.1:3000",
    "http://localhost:5173",
    "http://127.0.0.1:5173",
    "http://localhost:8000",
    "http://127.0.0.1:8000",
]
 
app.add_middleware(
    CORSMiddleware,
    allow_origins=origins,
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)
 
latest_data = {
    "distance": None,
    "tilt": None,
    "light": None,
    "water": None
}
 
@app.get("/")
def home():
    return {"message": "Trash can server is running"}
 
@app.get("/update")
def update(distance: int, tilt: int, light: int, water: int):
    latest_data["distance"] = distance
    latest_data["tilt"] = tilt
    latest_data["light"] = light
    latest_data["water"] = water
 
    print("Received data:", latest_data)
 
    return {
        "status": "ok",
        "received": latest_data
    }
 
@app.get("/latest")
def latest():
    return latest_data
 
 