/*
* IoT Hub sample for Intel Edison board - Microsoft Sample Code - Copyright (c) 2016 - Licensed MIT
*/
'use strict';

/**
 * Setup common gulp tasks: init, install-tools, deploy, run
 */
require('gulp-common')(require('gulp'), 'edison-c', {
  appName: 'lesson-1',
  configTemplate: {
    "device_host_name_or_ip_address": "[device hostname or IP adress]",
    "device_user_name": "root",
    "device_password": "[device password]"
  },
  configPostfix: "edison",
  app: [ 'main.c', 'CMakeLists.txt' ]
});
