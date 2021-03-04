const fs = require('fs');
const Module = require('../wasm_d/decoder');

const buffer = fs.readFileSync('src/encQVGA.h264');
const chunks = [];

let i = 0;
while (i < buffer.length) {
    if (buffer[i] === 1 && buffer[i - 1] === 0 && buffer[i - 2] === 0) {
        if (buffer[i - 3] === 0) {
            if (i > 4) {
                chunks.push(i - 3);
            }
        } else chunks.push(i - 2);
    }
    i++;
}
console.info('read video finish byteLen: %d frames: %d', buffer.byteLength, chunks.length);

Module.onRuntimeInitialized = () => {
    console.log('init wasm');

    Module._init();

    let start = 0;
    let end = 0;
    let current = 0;
    const timer = setInterval(() => {
        if (current >= chunks.length) {
            clearInterval(timer);
            console.log('Finished');
        } else {
            end = chunks[current];
            current++;

            const chunk = buffer.slice(start, end);
            console.log('send', current, chunk);

            const chunkPtr = Module._malloc(chunk.length);

            Module.HEAP8.set(chunk, chunkPtr);

            const imgDataPtr = Module._decode(chunkPtr, chunk.length);

            const width = Module.HEAPU32[imgDataPtr / 4],
                height = Module.HEAPU32[imgDataPtr / 4 + 1],
                imageBufferPtr = Module.HEAPU32[imgDataPtr / 4 + 2],
                imageBuffer = Module.HEAPU8.subarray(imageBufferPtr, imageBufferPtr + width * height * 3);

            Module._free(chunkPtr);
            Module._free(imgDataPtr);
            Module._free(imageBufferPtr);

            const imgInfo = { width, height, imageBuffer };

            const path = `src/ppm/${current}.ppm`;
            fs.writeFileSync(path, `P6\n${width} ${height}\n225\n`);
            fs.appendFileSync(path, imageBuffer);

            // console.log('ok', imgInfo);

            start = end;
        }
    }, 100);
};
