window.UI = {};

const THREE = require('three');
window.UI.THREE = THREE;

// console.log("window.UI.geometry", window.UI.geometry);

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
  const size = window.UI.geometry.attributes.position.count;
  return size;
}

window.UI.getIndexCount = function() {
  return window.UI.geometry.index.array.length / 3;
};

window.UI.getPosition = function() {
  return getFloatArray(window.UI.geometry.attributes.position.array);
};

window.UI.getUV = function() {
  return getFloatArray(window.UI.geometry.attributes.uv.array);
};

window.UI.getNormal = function() {
  return getFloatArray(window.UI.geometry.attributes.normal.array);
};

window.UI.getIndex = function() {
  return getIntArray(window.UI.geometry.index.array);
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
    //const width = Math.min(window.innerWidth, window.innerHeight);
    //const height = width;
    const width = window.innerWidth;
    const height = window.innerHeight;
    canv.width = width;
    canv.height = height;
    //console.log("resize: ", width, height);
    _setSize(width, height);
};

const handleKeyDown = function(evt) {
  if (evt.preventDefaulted) {
    return; // Do nothing if event already handled
  }
 
  let handled = false;
  switch(evt.code) {
    case "KeyS":
    case "ArrowDown":
      // Handle "back"
      _moveDown(true);
      handled = true;
      break;
    case "KeyW":
    case "ArrowUp":
      // Handle "forward"
      _moveUp(true);
      handled = true;
      break;
    case "KeyA":
    case "ArrowLeft":
      // Handle "turn left"
      _moveLeft(true);
      handled = true;
      break;
    case "KeyD":
    case "ArrowRight":
      // Handle "turn right"
      _moveRight(true);
      handled = true;
      break;
  }
 
  // Consume the event so it doesn't get handled twice
  if (handled) {
    evt.preventDefault();
  }
};

const handleKeyUp = function(evt) {
  if (evt.preventDefaulted) {
    return; // Do nothing if event already handled
  }
 
  let handled = false;
  switch(evt.code) {
    case "KeyS":
    case "ArrowDown":
      // Handle "back"
      _moveDown(false);
      handled = true;
      break;
    case "KeyW":
    case "ArrowUp":
      // Handle "forward"
      _moveUp(false);
      handled = true;
      break;
    case "KeyA":
    case "ArrowLeft":
      // Handle "turn left"
      _moveLeft(false);
      handled = true;
      break;
    case "KeyD":
    case "ArrowRight":
      // Handle "turn right"
      _moveRight(false);
      handled = true;
      break;
  }
 
  // Consume the event so it doesn't get handled twice
  if (handled) {
    evt.preventDefault();
  }
};

/*
let lastX = 0.5;
let lastY = 0.5;
*/

const handleMouseMove = function(evt) {
  /*
  let xpos = evt.clientX / canv.width;
  let ypos = evt.clientY / canv.height;
  */

  if(document.pointerLockElement === canv ||
    document.mozPointerLockElement === canv) {
  } else {
    return;
  }

  let xoffset = evt.movementX / canv.width;
  let yoffset = evt.movementY / canv.height;
  /*
  let xoffset = xpos - lastX;
  let yoffset = ypos - lastY;
  lastX = xpos;
  lastY = ypos;
  */

  const sensitivity = 100.0;
  xoffset *= sensitivity * 4.0;
  yoffset *= sensitivity;

  _addYawPitch(xoffset, yoffset); 
};

/*
const handleMouseEnterLeave = function(evt) {
  let xpos = evt.clientX / canv.width;
  let ypos = evt.clientY / canv.height;
  lastX = xpos;
  lastY = ypos;
}
*/

function allReady() {
  window.addEventListener("optimizedResize", resize);
  window.addEventListener("keydown", handleKeyDown, true);
  window.addEventListener("keyup", handleKeyUp, true);
  canv.addEventListener("mousemove", handleMouseMove, true);
  //canv.addEventListener("mouseenter", handleMouseEnterLeave, true);
  //canv.addEventListener("mouseleave", handleMouseEnterLeave, true);
  canv.addEventListener("mousedown", function(evt) {
    if (evt.button === 2) {
      canv.requestPointerLock();
    }
  }, false);
  canv.addEventListener("wheel", function(evt) {
    _addZoom(evt.deltaY * 45.0 / (100.0 * 20.0));
  }, true);
}
window.UI.allReady = allReady;
window.UI.resize = resize;
