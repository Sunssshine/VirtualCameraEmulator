from flask import Flask, render_template, request, send_from_directory, current_app
import codecs
import json
import os

app = Flask(__name__)
app.config["CACHE_TYPE"] = "null"

GENERATOR_BUILD_PATH = "./bin"

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

@app.route('/generate_data', methods=['GET', 'POST'])
def generateData():
    place = json.loads(request.data)
    file = codecs.open(f"{GENERATOR_BUILD_PATH}/parameters.json", "w", "utf-8")
    file.write(place)
    file.close()

    os.chdir(f"{GENERATOR_BUILD_PATH}")
    try:
        os.remove("result.mpg")
    except OSError:
        pass
    os.system('Generator.exe')
    os.chdir("../")
    return json.dumps({'success':True}), 200, {'ContentType':'application/json'}

@app.route('/uploads/<path:filename>', methods=['GET', 'POST'])
def download(filename):
    uploads = os.path.join(current_app.root_path, GENERATOR_BUILD_PATH)
    return send_from_directory(directory=uploads, filename=filename, as_attachment=True, cache_timeout=0)