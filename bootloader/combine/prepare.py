#!/usr/bin/python3
from __future__ import print_function
import argparse

parser = argparse.ArgumentParser(description='Commandline tool for combining bootloader with firmware for flashing.')
parser.add_argument('-b', '--bootloader', dest='blpath', required=True, help="Bootloader bin file")
parser.add_argument('-f', '--firmware', dest='fwpath', required=True, help="Firmware bin file to add")
parser.add_argument('-o', '--output', dest='combinedpath', required=True, help="Target image file")

args = parser.parse_args()

bl = open(args.blpath, 'rb').read()
fw = open(args.fwpath, 'rb').read()

combined = bl + fw[:256] + (32768-256)*b'\xFF' + fw[256:]

open(args.combinedpath, 'wb').write(combined)

print('bootloader : %d bytes' % len(bl))
print('firmware   : %d bytes' % len(fw))
print('combined   : %d bytes' % len(combined))
