/*
* IoT Hub sample for Intel Edison board - Microsoft Sample Code - Copyright (c) 2016 - Licensed MIT
*/
'use strict';

var eslint = require('gulp-eslint');
var gulp = require('gulp');

var cpplint = require('node-cpplint/lib/index');
var reporter = require('node-cpplint/lib/reporters').spec

gulp.task('lint', () => {
  return gulp.src([
    '**/*.js',
    '!**/node_modules/**',
  ])
    .pipe(eslint())
    .pipe(eslint.format())
    .pipe(eslint.failAfterError());
});

gulp.task('cpplint', () => {
  var options = {
    files: [
      './Lesson1/app/main.c',
      './Lesson3/app/main.c',
      './Lesson4/app/main.c'
    ],
    filters: {
      'whitespace': {
        'braces': false,
        'line_length': false,
        'newline': false
      },
      'build': {
        'include': false
      }
    },
    extensions: [
      'c'
    ]
  };

  cpplint(options, function (err, report) {
    reporter(err, report);

    // Return failure if there is any error.
    for (var key in report) {
      if (report.hasOwnProperty(key) && Array.isArray(report[key])) {
        if (report[key].length > 0) {
          process.exit(1);
        }
      }
    }
  });
})
