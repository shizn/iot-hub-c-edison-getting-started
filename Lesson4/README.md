---
services: iot-hub, iot-suite
platforms: C
author: ZhijunZhao
---

# Send device-to-cloud messages
This sample repo accompanies [Lesson 4: Send cloud-to-device messages]() lesson. It shows how to send messages from an Azure IoT hub to an Intel Edison board.

## Prerequisites
See [Lesson 4: Send cloud-to-device messages]() for more information.

## Repository information
- `app` sub-folder contains the sample C application that receives cloud-2-device messages and the CMakeLists.txt that builds the main.c source code.

## Running this sample
Please follow the [Lesson 4: Send cloud-to-device messages]() for detailed walkthrough of the steps below.

### Deploy and run

Install required npm packages on the host:
```bash
npm install
```
Create a JSON configuration file in the `.iot-hub-getting-started` sub-folder of the current user's home directory:
```bash
gulp init
```

Install required tools/packages on the Intel Edison board, deploy sample application, and run it on the device:
```bash
gulp install-tools
gulp deploy
gulp run
```
