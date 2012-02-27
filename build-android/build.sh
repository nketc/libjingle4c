cd ..
[ "x$NDK_BUILD" = "x" ] && NDK_BUILD=/opt/android-ndk-r6m-linux/ndk-build
/opt/android-ndk-r6m-linux/ndk-build TARGET_ARCH_ABI=$ARCH ANDROID_SRC_ROOT=/home/work/android2.2 APP_ABI=$ARCH -j4
[ "$?" = "0" ] || exit 1
cd - > /dev/null
REL_HEADERS="\
  ../cif/jinglep2p.h \
  ../cif/roster.h \
  ../cif/xmpp.h"
[ -e libjingle-$ARCH ] || mkdir libjingle-$ARCH
for h in $REL_HEADERS
do
  cp -f $h libjingle-$ARCH/
done
[ -e libjingle-$ARCH/lib-$ARCH ] || mkdir libjingle-$ARCH/lib-$ARCH
cp -f ../libs/$ARCH/libjingle.so libjingle-$ARCH/lib-$ARCH/
[ ! -e libjingle-${ARCH}.tar.bz2 ] || rm libjingle-${ARCH}.tar.bz2
tar cjvf libjingle-${ARCH}.tar.bz2 libjingle-$ARCH/ > /dev/null
rm -fr libjingle-$ARCH
echo "=========complete========="
