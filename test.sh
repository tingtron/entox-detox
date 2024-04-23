#!/bin/sh

dir='test'

echo entox - tokeninze DO - BA
echo ""

for p in $dir/*.DO ; do ./entox $p ${p%.*}.BA ; done

echo detox - de-tokeninze BA - D2
echo ""

for p in $dir/*.BA ; do ./detox $p ${p%.*}.D2 ; done

echo diff - compare DO - D2
echo ""

for p in $dir/*.DO ; do
    DF=$(diff $p ${p%.*}.D2)
    if [ "$DF" == "" ] ; then
        rm ${p%.*}.BA ${p%.*}.D2
    else
        echo No match: $p ${p%.*}.D2
        echo $DF
    fi
done
