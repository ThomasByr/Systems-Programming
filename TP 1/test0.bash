#!/usr/bin/env bash
# -*- coding: utf-8 -*-

mkdir tmp
cp /bin/ls tmp/toto
sed "s/\(.\{4031\}\)./\1/" tmp/toto >tmp/titi

cmp tmp/toto tmp/titi

rm -R tmp
