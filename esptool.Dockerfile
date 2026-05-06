FROM python:3.12-slim

RUN pip install esptool

ENTRYPOINT ["python", "-m", "esptool"]
