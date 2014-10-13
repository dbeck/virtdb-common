var gulp = require('gulp');
var coffee = require('gulp-coffee');
var spawn = require('child_process').spawn;
var sourcemaps = require('gulp-sourcemaps');
var gulp = require('gulp');
var coffee = require('gulp-coffee');
var spawn = require('child_process').spawn;
var sourcemaps = require('gulp-sourcemaps');
var mocha = require('gulp-mocha');
require('coffee-script/register')
var gulp = require("gulp");
var coffee = require("gulp-coffee");
var spawn = require("child_process").spawn;
var sourcemaps = require("gulp-sourcemaps");
var node;

/**
 * $ gulp server
 * description: launch the server. If there's a server already running, kill it.
 */
gulp.task("coffee", function() {
    gulp.src("*.coffee")
        .pipe(sourcemaps.init())
        .pipe(coffee({bare: true}))
        .pipe(sourcemaps.write("."))
        .pipe(gulp.dest("./lib"))
});

gulp.task("collect-proto", function() {
    gulp.src("../proto/*.desc")
        .pipe(gulp.dest("./lib/proto"))
});

gulp.task('watch', ['coffee', 'collect-proto', 'test'], function()
{
    gulp.watch(['./*.coffee'], ['coffee', 'test']);
    gulp.watch(['test/*.coffee'], ['test']);
});

gulp.task('test', ['coffee'], function ()
{
    return gulp.src('test/*.coffee', {read: false})
            .pipe(mocha({reporter: 'min'}));
})


gulp.task("default", ["watch"]);
