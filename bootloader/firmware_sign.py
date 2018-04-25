#!/usr/bin/python
from __future__ import print_function
import argparse
import hashlib
import struct
import binascii
import ecdsa

try:
    raw_input
except:
    raw_input = input

SLOTS = 3

pubkeys = {
    1: '042ff357e880474842268886b5c436285c9afc154404ba89a2452dd2d1580839902bb58a766f1e2e5d862d1656d2d0609077a4edee94111d6c7db8d3ba5e42425f',
    2: '04a3734e08f5a8a3402ffa511df7d05acc0b38062064b266c1c8ba2d1463840b6e04bc844e5dce4c8f0583be16bffa6b6e8163476fa845ddf097cf5f38301e217f',
    3: '04fa06b9afda236ee227812f8c34925e19224d19b3826abb25ffb89e09eb171b905da0875ed09c8146ec6ef06994f41446719961a81772e3a5aea9db581fab1694',
    4: '0446ce6819f8d155fe022b540aba41456734ab631125b12705da1cb1e8fedece290db8291c08e5b9aa80f41466bd79d292b5b03328bc4dbfbe461c9271c54d7ed7',
    5: '04506a11575e4d0f7aca6dcebf4df31b52832ea68b972b5721a8f91a89dbbe7a7539bff4a3dc505f0e214aba812e80c9280cf48ac62b0659eaf7791111f15ee681',
}

INDEXES_START = len('SAFT') + struct.calcsize('<I')
SIG_START = INDEXES_START + SLOTS + 1 + 52

def parse_args():
    parser = argparse.ArgumentParser(description='Commandline tool for signing Trezor firmware.')
    parser.add_argument('-f', '--file', dest='path', help="Firmware file to modify")
    parser.add_argument('-s', '--sign', dest='sign', action='store_true', help="Add signature to firmware slot")
    parser.add_argument('-p', '--pem', dest='pem', action='store_true', help="Use PEM instead of SECEXP")
    parser.add_argument('-g', '--generate', dest='generate', action='store_true', help='Generate new ECDSA keypair')

    return parser.parse_args()

def prepare(data):
    # Takes raw OR signed firmware and clean out metadata structure
    # This produces 'clean' data for signing

    meta = b'SAFT'  # magic
    if data[:4] == b'SAFT':
        meta += data[4:4 + struct.calcsize('<I')]
    else:
        meta += struct.pack('<I', len(data))  # length of the code
    meta += b'\x00' * SLOTS  # signature index #1-#3
    meta += b'\x01'       # flags
    meta += b'\x00' * 52  # reserved
    meta += b'\x00' * 64 * SLOTS  # signature #1-#3

    if data[:4] == b'SAFT':
        # Replace existing header
        out = meta + data[len(meta):]
    else:
        # create data from meta + code
        out = meta + data

    return out

def check_signatures(data):
    # Analyses given firmware and prints out
    # status of included signatures

    try:
        indexes = [ ord(x) for x in data[INDEXES_START:INDEXES_START + SLOTS] ]
    except:
        indexes = [ x for x in data[INDEXES_START:INDEXES_START + SLOTS] ]

    to_sign = prepare(data)[256:] # without meta
    fingerprint = hashlib.sha256(to_sign).hexdigest()

    print("Firmware fingerprint:", fingerprint)

    used = []
    for x in range(SLOTS):
        signature = data[SIG_START + 64 * x:SIG_START + 64 * x + 64]

        if indexes[x] == 0:
            print("Slot #%d" % (x + 1), 'is empty')
        else:
            pk = pubkeys[indexes[x]]
            verify = ecdsa.VerifyingKey.from_string(binascii.unhexlify(pk)[1:],
                        curve=ecdsa.curves.SECP256k1, hashfunc=hashlib.sha256)

            try:
                verify.verify(signature, to_sign, hashfunc=hashlib.sha256)

                if indexes[x] in used:
                    print("Slot #%d signature: DUPLICATE" % (x + 1), binascii.hexlify(signature))
                else:
                    used.append(indexes[x])
                    print("Slot #%d signature: VALID" % (x + 1), binascii.hexlify(signature))

            except:
                print("Slot #%d signature: INVALID" % (x + 1), binascii.hexlify(signature))


def modify(data, slot, index, signature):
    # Replace signature in data

    # Put index to data
    data = data[:INDEXES_START + slot - 1 ] + chr(index) + data[INDEXES_START + slot:]

    # Put signature to data
    data = data[:SIG_START + 64 * (slot - 1) ] + signature + data[SIG_START + 64 * slot:]

    return data

def sign(data, is_pem):
    # Ask for index and private key and signs the firmware

    slot = int(raw_input('Enter signature slot (1-%d): ' % SLOTS))
    if slot < 1 or slot > SLOTS:
        raise Exception("Invalid slot")

    if is_pem:
        print("Paste ECDSA private key in PEM format and press Enter:")
        print("(blank private key removes the signature on given index)")
        pem_key = ''
        while True:
            key = raw_input()
            pem_key += key + "\n"
            if key == '':
                break
        if pem_key.strip() == '':
            # Blank key,let's remove existing signature from slot
            return modify(data, slot, 0, '\x00' * 64)
        key = ecdsa.SigningKey.from_pem(pem_key)
    else:
        print("Paste SECEXP (in hex) and press Enter:")
        print("(blank private key removes the signature on given index)")
        secexp = raw_input()
        if secexp.strip() == '':
            # Blank key,let's remove existing signature from slot
            return modify(data, slot, 0, '\x00' * 64)
        key = ecdsa.SigningKey.from_secret_exponent(secexp = int(secexp, 16), curve=ecdsa.curves.SECP256k1, hashfunc=hashlib.sha256)

    to_sign = prepare(data)[256:] # without meta

    # Locate proper index of current signing key
    pubkey = b'04' + binascii.hexlify(key.get_verifying_key().to_string())
    index = None
    for i, pk in pubkeys.items():
        if pk == pubkey:
            index = i
            break

    if index == None:
        raise Exception("Unable to find private key index. Unknown private key?")

    signature = key.sign_deterministic(to_sign, hashfunc=hashlib.sha256)

    return modify(data, slot, index, signature)

def main(args):
    if args.generate:
        key = ecdsa.SigningKey.generate(
            curve=ecdsa.curves.SECP256k1,
            hashfunc=hashlib.sha256)

        print("PRIVATE KEY (SECEXP):")
        print(binascii.hexlify(key.to_string()))
        print()

        print("PRIVATE KEY (PEM):")
        print(key.to_pem())

        print("PUBLIC KEY:")
        print('04' + binascii.hexlify(key.get_verifying_key().to_string()))
        return

    if not args.path:
        raise Exception("-f/--file is required")

    data = open(args.path, 'rb').read()
    assert len(data) % 4 == 0

    if data[:4] != b'SAFT':
        print("Metadata has been added...")
        data = prepare(data)

    if data[:4] != b'SAFT':
        raise Exception("Firmware header expected")

    print("Firmware size %d bytes" % len(data))

    check_signatures(data)

    if args.sign:
        data = sign(data, args.pem)
        check_signatures(data)

    fp = open(args.path, 'wb')
    fp.write(data)
    fp.close()

if __name__ == '__main__':
    args = parse_args()
    main(args)
