utfdocs=`ls utf-8/*.txt`
for f in $utfdocs
do
  n=`basename $f`
  echo "converting encoding for $f ..."
  iconv -f UTF-8 -t GBK $f > gbk/$n
  unix2dos gbk/$n
done
