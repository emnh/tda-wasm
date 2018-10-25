(function() {
    var throttle = function(type, name, obj) {
        obj = obj || window;
        var running = false;
        var func = function() {
            if (running) { return; }
            running = true;
             requestAnimationFrame(function() {
                obj.dispatchEvent(new CustomEvent(name));
                running = false;
            });
        };
        obj.addEventListener(type, func);
    };

    /* init - you can init any event */
    throttle("resize", "optimizedResize");
})();

let canv = document.getElementById('canvas');

// handle event
const resize = function() {
    const width = Math.min(window.innerWidth, window.innerHeight);
    const height = width;
    canv.width = width;
    canv.height = height;
    //console.log("resize: ", width, height);
    _setSize(width, height);
};

function allReady() {
  resize();
  canv.addEventListener('click',    _toggle_background_color, false);
  canv.addEventListener('touchend', _toggle_background_color, false);
  window.addEventListener("optimizedResize", resize);
}
