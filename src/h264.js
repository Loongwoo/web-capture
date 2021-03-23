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

    const buff = new Uint8Array([1]); // colorTransform
    const buffPtr = Module._malloc(buff.length);
    Module.HEAPU8.set(buff, buffPtr);
    Module._init(buffPtr, buff.length);

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
            console.log('send', current, chunk[4] & 0x1f, chunk);

            const chunkPtr = Module._malloc(chunk.length);

            Module.HEAP8.set(chunk, chunkPtr);

            const imgDataPtr = Module._decode(chunkPtr, chunk.length);

            const width = Module.HEAPU32[imgDataPtr / 4],
                height = Module.HEAPU32[imgDataPtr / 4 + 1],
                length = Module.HEAPU32[imgDataPtr / 4 + 2],
                imagePtr = Module.HEAPU32[imgDataPtr / 4 + 3],
                imageBuffer = Module.HEAPU8.subarray(imagePtr, imagePtr + length);

            Module._free(chunkPtr);
            Module._free(imgDataPtr);
            Module._free(imagePtr);

            if (width > 0 && height > 0) {
                const path = `src/ppm/${current}.ppm`;
                fs.writeFileSync(path, `P6\n${width} ${height}\n225\n`);
                fs.appendFileSync(path, imageBuffer);
            }

            start = end;
        }
    }, 100);

    Module._free(buffPtr);
};
