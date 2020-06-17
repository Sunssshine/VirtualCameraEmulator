# VirtualCameraEmulator
Virtual camera emulator for simulation real camera image distortion effects for testing CV algorithms

## Инструкция по запуску

#### Запуск flask сервера
1. Перейти в директорию `server`
2. Создать виртуальное окружение командой `py -3 -m venv venv`
3. Активировать виртуальное окружение `venv\Scripts\activate`
4. Установить `Flask` командой `pip install Flask`
2. Установить переменные окружения
    ```
    FLASK_APP=app.py
    FLASK_ENV=development
    FLASK_DEBUG=0
    ```

3. Убедиться в том, что порт 5000 свободен и не используется другими приложениями в данный момент
4. Запустить сервер командой `python -m flask run`
5. Открыть браузер и перейти по адресу [http://localhost:5000](http://localhost:5000)

## Использование
WIP

## Структура репозитория
WIP