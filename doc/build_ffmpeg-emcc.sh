echo "===== start build ffmpeg-emcc ====="

NOW_PATH=$(cd $(dirname $0); pwd)

WEB_CAPTURE_PATH=$(cd $NOW_PATH/../; pwd)

FFMPEG_PATH=$(cd $WEB_CAPTURE_PATH/../ffmpeg-3.4.8; pwd)

rm -rf  $WEB_CAPTURE_PATH/lib/ffmpeg-emcc

mkdir $WEB_CAPTURE_PATH/lib/ffmpeg-emcc

cd $FFMPEG_PATH

make clean

emconfigure ./configure \
    --prefix=$WEB_CAPTURE_PATH/lib/ffmpeg-emcc \
    --cc="emcc" \
    --cxx="em++" \
    --ar="emar" \
    --cpu=generic \
    --target-os=none \
    --arch=x86_32 \
    --enable-gpl \
    --enable-version3 \
    --enable-small \
    --disable-avdevice \
    --enable-cross-compile \
    --disable-logging \
    --disable-programs \
    --disable-ffmpeg \
    --disable-ffplay \
    --disable-ffprobe \
    --disable-ffserver \
    --disable-doc \
    --disable-swresample \
    --disable-postproc  \
    --disable-avfilter \
    --disable-pthreads \
    --disable-w32threads \
    --disable-os2threads \
    --disable-network \
    --disable-everything \
    --disable-protocol=file \
    --disable-demuxer=mov \
    --disable-demuxer=matroska \
    --disable-demuxer=flv \
    --disable-demuxer=avi \
    --enable-decoder=h264 \
    --disable-decoder=hevc \
    --disable-decoder=mpeg4 \
    --disable-decoder=vp8 \
    --disable-decoder=vp9 \
    --disable-decoder=wmv3 \
    --disable-asm \
    --disable-debug \

make -j4

make install

echo "===== build ffmpeg-emcc finished  ====="