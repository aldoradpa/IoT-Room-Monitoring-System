# IoT Room Monitoring System

## Overview

IoT-based room monitoring system designed to monitor temperature, humidity, and gas concentration in real time. This project was developed using ESP8266, DHT22, MQ135, Blynk, and Telegram integration to help maintain ideal environmental conditions for storage areas.

## Features

* Real-time temperature monitoring
* Real-time humidity monitoring
* Gas concentration monitoring
* Automatic fan control based on environmental conditions
* LCD display for local monitoring
* Telegram notifications for abnormal conditions
* Remote monitoring using Blynk

## Hardware Components

* NodeMCU ESP8266
* DHT22 Temperature & Humidity Sensor
* MQ135 Gas Sensor
* LCD I2C 16x2
* Relay Module
* Cooling Fans
* LED Indicator
* Power Supply

## Software & Tools

* Arduino IDE
* Blynk IoT Platform
* Telegram Bot API
* ESP8266 Library

## System Workflow

1. ESP8266 initializes sensors and network connection.
2. DHT22 reads temperature and humidity data.
3. MQ135 reads gas concentration data.
4. Sensor data is displayed on the LCD.
5. Data is sent to Blynk for remote monitoring.
6. The system evaluates environmental conditions.
7. Fans and indicators are controlled automatically.
8. Telegram notifications are sent when abnormal conditions are detected.
9. The monitoring process repeats continuously.

## Project Structure

```text
Arduino/
Diagram/
Screenshot/
README.md
```

## Diagrams

* Flowchart System
* Block Diagram
* Wiring Diagram

## Screenshots

### Dashboard Monitoring

(Add dashboard image here)

### Telegram Notifications

(Add notification image here)

### LCD Display

(Add LCD image here)

### Prototype Device

(Add prototype image here)

## Technologies

* C++
* Arduino IDE
* ESP8266
* IoT
* Blynk
* Telegram Bot

## Author

Aldo Raditya Pangestu

Computer Systems Graduate with interests in IoT, Embedded Systems, and Software Development.
