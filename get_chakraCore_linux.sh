#!/bin/sh

cd 3rdParty/prebuilt

curl -SL https://aka.ms/chakracore/install | bash

cp ChakraCoreFiles/lib/libChakraCore.so ChakraCore/x64_release/libChakraCore.so


