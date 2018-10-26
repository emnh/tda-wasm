window.UI = {};


const THREE = require('three');

const mainGeometry = new THREE.BoxBufferGeometry(1, 1, 1, 1);

console.log("mainGeometry", mainGeometry);

function getFloatArray(ar) {
  const fsz = 4;
  const size = ar.length;
  const offset = Module._malloc(size * fsz);
  const doublePtr = Module.HEAPF32.subarray(offset / fsz, offset / fsz + size);
  for (let i = 0; i < size; i++) {
    doublePtr[i] = ar[i];
  }
  return offset;
}

function getIntArray(ar) {
  const fsz = 4;
  const size = ar.length;
  const offset = Module._malloc(size * fsz);
  const intPtr = Module.HEAP32.subarray(offset / fsz, offset / fsz + size);
  for (let i = 0; i < size; i++) {
    intPtr[i] = ar[i];
  }
  return offset;
}

window.UI.getPositionCount = function() {
  const size = mainGeometry.attributes.position.count;
  return size;
}

window.UI.getIndexCount = function() {
  return mainGeometry.index.array.length / 3;
};

window.UI.getPosition = function() {
  return getFloatArray(mainGeometry.attributes.position.array);
};

window.UI.getUV = function() {
  return getFloatArray(mainGeometry.attributes.uv.array);
};


window.UI.getIndex = function() {
  return getIntArray(mainGeometry.index.array);
};

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
window.UI.allReady = allReady;
