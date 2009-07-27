#!/bin/sh

rm -f "configure"
autoconf
rm -r -f "autom4te.cache"
