echo "===== start build wasm ====="

NOW_PATH=$(cd $(dirname $0); pwd)

WEB_CAPTURE_PATH=$(cd $NOW_PATH/../; pwd)

FFMPEG_PATH=$(cd $WEB_CAPTURE_PATH/lib/ffmpeg-emcc; pwd)

CLIB_PATH=$(cd $WEB_CAPTURE_PATH/clib/; pwd)

TOTAL_MEMORY=33554432

rm -rf $WEB_CAPTURE_PATH/wasm_d

mkdir $WEB_CAPTURE_PATH/wasm_d

emcc $CLIB_PATH/decoder.c $FFMPEG_PATH/lib/libavformat.a $FFMPEG_PATH/lib/libavcodec.a $FFMPEG_PATH/lib/libswscale.a $FFMPEG_PATH/lib/libavutil.a \
    -O3 \
    -I "$FFMPEG_PATH/include" \
    -s WASM=1 \
    -s TOTAL_MEMORY=$TOTAL_MEMORY \
    -s EXPORTED_FUNCTIONS='["_init", "_free", "_decode", "_destroy"]' \
    -s ASSERTIONS=0 \
    -s ALLOW_MEMORY_GROWTH=1 \
    -s MAXIMUM_MEMORY=200MB \
    -s ENVIRONMENT=node \
    -o $WEB_CAPTURE_PATH/wasm_d/decoder.js

echo "===== build wasm finished  ====="
