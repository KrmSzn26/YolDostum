{# app.py
from flask import Flask, request
import requests

app = Flask(_name_)

@app.route('/konum', methods=['POST'])
def konum_guncelle():
    try:
        data = request.get_json()
        lat = data.get('lat')
        lng = data.get('lng')

        # Firebase'e veri gönder (https ile)
        firebase_url = "https://aractakip-530ea-default-rtdb.firebaseio.com/vehicles/arac1/location.json"
        response = requests.put(firebase_url, json={"lat": lat, "lng": lng})

        return "Firebase'e başarıyla gönderildi!", 200
    except Exception as e:
        return f"Hata: {str(e)}", 500
# /var/www/kullanici_adin_pythonanywhere_com_wsgi.py

import sys
path = '/home/kullanici_adin'
if path not in sys.path:
    sys.path.append(path)

from app import app as application
}
