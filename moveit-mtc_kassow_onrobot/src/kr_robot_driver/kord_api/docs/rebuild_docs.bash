#!/bin/bash

DELETE_FOLDERS=(
    _doxy_out/
    _static/
    _build/
)

KORD_API_DOC_TARBALL=kord_api_doc.tgz

for item in ${DELETE_FOLDERS[@]}; do
echo "Checking: $item"
    if [ -d $item ]; then
        echo "Deleting: $item"
        rm -r $item
    fi
    
    if [ -e $KORD_API_DOC_TARBALL ]; then
        echo "Deleting tarball"
        rm $KORD_API_DOC_TARBALL
    fi
done

if [ "$1" == "clean" ]; then
    echo "Clean up only"
    exit 0
fi

echo "Compiling the documentation"

doxygen && mkdir _static && make html 
ret=$?

if [ "$1" == "deploy" ]; then    
    if [ $ret -eq 0 ]; then
        echo "Package the output to tarball"
        tar -czvf $KORD_API_DOC_TARBALL _build/html/
    fi
fi

exit 0
