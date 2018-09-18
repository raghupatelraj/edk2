# @file
#   Linux script file to generate UEFI capsules for system firmware
#
# Copyright (c) 2018, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#

FMP_CAPSULE_VENDOR=Intel
FMP_CAPSULE_GUID=4096267B-DA0A-42EB-B5EB-FEF31D207CB4
FMP_CAPSULE_FILE=MinnowMaxRelease.cap
FMP_CAPSULE_VERSION=0x00000008
FMP_CAPSULE_STRING=0.0.0.8
FMP_CAPSULE_NAME="Intel MinnowMax RELEASE UEFI $FMP_CAPSULE_STRING"
FMP_CAPSULE_LSV=0x00000000
FMP_CAPSULE_PAYLOAD=$WORKSPACE/Build/Vlv2TbltDevicePkg/RELEASE_GCC49/FV/Vlv.ROM

if [ ! -e "$FMP_CAPSULE_PAYLOAD" ] ; then
  return
fi

if [ -e NewCert.pem ]; then
  #
  # Sign capsule using OpenSSL with a new certificate
  #
  GenerateCapsule \
    --encode \
    -v \
    --guid $FMP_CAPSULE_GUID \
    --fw-version $FMP_CAPSULE_VERSION \
    --lsv $FMP_CAPSULE_LSV \
    --capflag PersistAcrossReset \
    --capflag InitiateReset \
    --signer-private-cert=NewCert.pem \
    --other-public-cert=NewSub.pub.pem \
    --trusted-public-cert=NewRoot.pub.pem \
    -o $FMP_CAPSULE_FILE \
    $FMP_CAPSULE_PAYLOAD

  cp $FMP_CAPSULE_FILE $WORKSPACE/Build/Vlv2TbltDevicePkg/Capsules/NewCert

  rm $FMP_CAPSULE_FILE
fi

#
# Sign capsule using OpenSSL with EDK II Test Certificate
#
GenerateCapsule \
  --encode \
  -v \
  --guid $FMP_CAPSULE_GUID \
  --fw-version $FMP_CAPSULE_VERSION \
  --lsv $FMP_CAPSULE_LSV \
  --capflag PersistAcrossReset \
  --capflag InitiateReset \
  --signer-private-cert=$WORKSPACE/edk2/BaseTools/Source/Python/Pkcs7Sign/TestCert.pem \
  --other-public-cert=$WORKSPACE/edk2/BaseTools/Source/Python/Pkcs7Sign/TestSub.pub.pem \
  --trusted-public-cert=$WORKSPACE/edk2/BaseTools/Source/Python/Pkcs7Sign/TestRoot.pub.pem \
  -o $FMP_CAPSULE_FILE \
  $FMP_CAPSULE_PAYLOAD

cp $FMP_CAPSULE_FILE $WORKSPACE/Build/Vlv2TbltDevicePkg/Capsules/TestCert

rm $FMP_CAPSULE_FILE

