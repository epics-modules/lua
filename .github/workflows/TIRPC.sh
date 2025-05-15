#!/bin/bash
set -e

echo "TIRPC=YES" >> ./configure/CONFIG_SITE.local
echo "DRV_VXI11=YES" >> ./configure/CONFIG_SITE.local
