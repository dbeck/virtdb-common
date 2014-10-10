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

gulp.task("watch", ["coffee", "collect-proto"], function()
{
    gulp.watch(["./*.coffee"], ["coffee"]);
});

gulp.task("default", ["watch"]);
