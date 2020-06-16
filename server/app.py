from flask import Flask
from flask import render_template

app = Flask(__name__)

@app.route('/')
def start_page():
    return render_template('start.html')

@app.route('/generate/')
def generate():
    return render_template('generate.html')

@app.route('/processing/')
def processing():
    return render_template('processing.html')

@app.route('/result/')
def result():
    return render_template('result.html')