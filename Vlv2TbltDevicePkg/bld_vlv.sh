#!/usr/bin/env bash
##**********************************************************************
## Function define
##**********************************************************************
function Usage() {
  echo
  echo "***************************************************************************"
  echo "Build BIOS rom for VLV platforms."
  echo
  echo "Usage: bld_vlv.bat  PlatformType [Build Target]"
  echo
  echo
  echo "       Platform Types:  MNW2"
  echo "       Build Targets:   Debug, Release  (default: Debug)"
  echo
  echo "***************************************************************************"
  echo "Press any key......"
  read
  exit 0
}


echo -e $(date)
##**********************************************************************
## Initial Setup
##**********************************************************************
#WORKSPACE=$(pwd)
#build_threads=($NUMBER_OF_PROCESSORS)+1
Build_Flags=
exitCode=0
Arch=X64
SpiLock=0

## Clean up previous build files.
if [ -e $(pwd)/EDK2.log ]; then
  rm $(pwd)/EDK2.log
fi

if [ -e $(pwd)/Unitool.log ]; then
  rm $(pwd)/Unitool.log
fi

if [ -e $WORKSPACE/Conf/target.txt ]; then
  rm $WORKSPACE/Conf/target.txt
fi

if [ -e $WORKSPACE/Conf/BiosId.env ]; then
  rm $WORKSPACE/Conf/BiosId.env
fi

if [ -e $WORKSPACE/Conf/tools_def.txt ]; then
  rm $WORKSPACE/Conf/tools_def.txt
fi

if [ -e $WORKSPACE/Conf/build_rule.txt ]; then
  rm $WORKSPACE/Conf/build_rule.txt
fi


## Setup EDK environment. Edksetup puts new copies of target.txt, tools_def.txt, build_rule.txt in WorkSpace\Conf
## Also run edksetup as soon as possible to avoid it from changing environment variables we're overriding
. edksetup.sh BaseTools
make -C BaseTools

## Define platform specific environment variables.
PLATFORM_PACKAGE=Vlv2TbltDevicePkg
config_file=./$PLATFORM_PACKAGE/PlatformPkgConfig.dsc
auto_config_inc=./$PLATFORM_PACKAGE/AutoPlatformCFG.txt

## default ECP (override with /ECP flag)
# EDK_SOURCE=$WORKSPACE/EdkCompatibilityPkg

## create new AutoPlatformCFG.txt file
if [ -f "$auto_config_inc" ]; then
  rm $auto_config_inc
fi
touch $auto_config_inc

##**********************************************************************
## Parse command line arguments
##**********************************************************************

## Optional arguments
for (( i=1; i<=$#; ))
  do
    if [ "$1" == "/?" ]; then
      Usage
    elif [ "$(echo $1 | tr 'a-z' 'A-Z')" == "/Q" ]; then
      Build_Flags="$Build_Flags --quiet"
      shift
    elif [ "$(echo $1 | tr 'a-z' 'A-Z')" == "/L" ]; then
      Build_Flags="$Build_Flags -j EKD2.log"
      shift
    elif [ "$(echo $1 | tr 'a-z' 'A-Z')" == "/C" ]; then
      echo Removing previous build files ...
      if [ -d "Build" ]; then
        rm -r Build
      fi
      shift
    elif [ "$(echo $1 | tr 'a-z' 'A-Z')" == "/ECP" ]; then
      ECP_SOURCE=$WORKSPACE/EdkCompatibilityPkgEcp
      EDK_SOURCE=$WORKSPACE/EdkCompatibilityPkgEcp
      echo DEFINE ECP_BUILD_ENABLE = TRUE >> $auto_config_inc
      shift
    elif [ "$(echo $1 | tr 'a-z' 'A-Z')" == "/X64" ]; then
      Arch=X64
      shift
    elif [ "$(echo $1 | tr 'a-z' 'A-Z')" == "/YL" ]; then
      SpiLock=1
      shift      
    else
      break
    fi
  done





## Required argument(s)
if [ "$2" == "" ]; then
  Usage
fi

## Remove the values for Platform_Type and Build_Target from BiosIdX.env and stage in Conf
if [ $Arch == "IA32" ]; then
  cp $PLATFORM_PACKAGE/BiosIdR.env    $WORKSPACE/Conf/BiosId.env
  echo DEFINE X64_CONFIG = FALSE      >> $auto_config_inc
else
  cp $PLATFORM_PACKAGE/BiosIdx64R.env  $WORKSPACE/Conf/BiosId.env
  echo DEFINE X64_CONFIG = TRUE       >> $auto_config_inc
fi
sed -i '/^BOARD_ID/d' $WORKSPACE/Conf/BiosId.env
sed -i '/^BUILD_TYPE/d' $WORKSPACE/Conf/BiosId.env



## -- Build flags settings for each Platform --
##    AlpineValley (ALPV):  SVP_PF_BUILD = TRUE,   ENBDT_PF_BUILD = FALSE,  TABLET_PF_BUILD = FALSE,  BYTI_PF_BUILD = FALSE, IVI_PF_BUILD = FALSE
##       BayleyBay (BBAY):  SVP_PF_BUILD = FALSE,  ENBDT_PF_BUILD = TRUE,   TABLET_PF_BUILD = FALSE,  BYTI_PF_BUILD = FALSE, IVI_PF_BUILD = FALSE
##         BayLake (BLAK):  SVP_PF_BUILD = FALSE,  ENBDT_PF_BUILD = FALSE,  TABLET_PF_BUILD = TRUE,   BYTI_PF_BUILD = FALSE, IVI_PF_BUILD = FALSE
##      Bakersport (BYTI):  SVP_PF_BUILD = FALSE,  ENBDT_PF_BUILD = FALSE,  TABLET_PF_BUILD = FALSE,  BYTI_PF_BUILD = TRUE, IVI_PF_BUILD = FALSE
## Crestview Hills (CVHS):  SVP_PF_BUILD = FALSE,  ENBDT_PF_BUILD = FALSE,  TABLET_PF_BUILD = FALSE,  BYTI_PF_BUILD = TRUE, IVI_PF_BUILD = TRUE
##            FFD8 (BLAK):  SVP_PF_BUILD = FALSE,  ENBDT_PF_BUILD = FALSE,  TABLET_PF_BUILD = TRUE,   BYTI_PF_BUILD = FALSE, IVI_PF_BUILD = FALSE
echo "Setting  $1  platform configuration and BIOS ID..."
if [ "$(echo $1 | tr 'a-z' 'A-Z')" == "MNW2" ]; then
  echo BOARD_ID = MNW2MAX             >> $WORKSPACE/Conf/BiosId.env
  echo DEFINE ENBDT_PF_BUILD = TRUE  >> $auto_config_inc
else
  echo "Error - Unsupported PlatformType: $1"
  Usage
fi

Platform_Type=$1

if [ "$(echo $2 | tr 'a-z' 'A-Z')" == "RELEASE" ]; then
  TARGET=RELEASE
  BUILD_TYPE=R
  echo BUILD_TYPE = R >> $WORKSPACE/Conf/BiosId.env
else
  TARGET=DEBUG
  BUILD_TYPE=D
  echo BUILD_TYPE = D >> $WORKSPACE/Conf/BiosId.env
fi


##**********************************************************************
## Additional EDK Build Setup/Configuration
##**********************************************************************
echo "Ensuring correct build directory is present for GenBiosId..."

echo Modifing Conf files for this build...
## Remove lines with these tags from target.txt
sed -i '/^ACTIVE_PLATFORM/d' $WORKSPACE/Conf/target.txt
sed -i '/^TARGET /d' $WORKSPACE/Conf/target.txt
sed -i '/^TARGET_ARCH/d' $WORKSPACE/Conf/target.txt
sed -i '/^TOOL_CHAIN_TAG/d' $WORKSPACE/Conf/target.txt
sed -i '/^MAX_CONCURRENT_THREAD_NUMBER/d' $WORKSPACE/Conf/target.txt

gcc_version=$(gcc -v 2>&1 | tail -1 | awk '{print $3}')
case $gcc_version in
    4.5.*)
      TARGET_TOOLS=GCC45
      ;;
    4.6.*)
      TARGET_TOOLS=GCC46
      ;;
    4.7.*)
      TARGET_TOOLS=GCC47
      ;;
    4.8.*)
      TARGET_TOOLS=GCC48
      ;;
    4.9.*|4.1[0-9].*|5.*.*|6.*.*)
      TARGET_TOOLS=GCC49
      ;;
    *)
      TARGET_TOOLS=GCC44
      ;;
esac

ACTIVE_PLATFORM=$PLATFORM_PACKAGE/PlatformPkgGcc"$Arch".dsc
TOOL_CHAIN_TAG=$TARGET_TOOLS
MAX_CONCURRENT_THREAD_NUMBER=1
echo ACTIVE_PLATFORM = $ACTIVE_PLATFORM                           >> $WORKSPACE/Conf/target.txt
echo TARGET          = $TARGET                                    >> $WORKSPACE/Conf/target.txt
echo TOOL_CHAIN_TAG  = $TOOL_CHAIN_TAG                            >> $WORKSPACE/Conf/target.txt
echo MAX_CONCURRENT_THREAD_NUMBER = $MAX_CONCURRENT_THREAD_NUMBER >> $WORKSPACE/Conf/target.txt
if [ $Arch == "IA32" ]; then
  echo TARGET_ARCH   = IA32                                       >> $WORKSPACE/Conf/target.txt
else
  echo TARGET_ARCH   = IA32 X64                                   >> $WORKSPACE/Conf/target.txt
fi

##**********************************************************************
## Build BIOS
##**********************************************************************
echo Skip "Running UniTool..."
echo "Make GenBiosId Tool..."
BUILD_PATH=$WORKSPACE/Build/$PLATFORM_PACKAGE/"$TARGET"_"$TOOL_CHAIN_TAG"
if [ ! -d "$BUILD_PATH/$Arch" ]; then
  mkdir -p $BUILD_PATH/$Arch
fi
if [ -e "$BUILD_PATH/$Arch/BiosId.bin" ]; then
  rm -f $BUILD_PATH/$Arch/BiosId.bin
fi


./$PLATFORM_PACKAGE/GenBiosId -i $WORKSPACE/Conf/BiosId.env -o $BUILD_PATH/$Arch/BiosId.bin


echo "Invoking EDK2 build..."
build

if [ $SpiLock == "1" ]; then
  IFWI_HEADER_FILE=$WORKSPACE/edk2/$PLATFORM_PACKAGE/Stitch/IFWIHeader/IFWI_HEADER_SPILOCK.bin
else
  IFWI_HEADER_FILE=$WORKSPACE/edk2/$PLATFORM_PACKAGE/Stitch/IFWIHeader/IFWI_HEADER.bin
fi

echo $IFWI_HEADER_FILE

##**********************************************************************
## Post Build processing and cleanup
##**********************************************************************

echo Skip "Running fce..."

echo Skip "Running KeyEnroll..."

## Set the Board_Id, Build_Type, Version_Major, and Version_Minor environment variables
VERSION_MAJOR=$(grep '^VERSION_MAJOR' $WORKSPACE/Conf/BiosId.env | cut -d ' ' -f 3 | cut -c 1-4)
VERSION_MINOR=$(grep '^VERSION_MINOR' $WORKSPACE/Conf/BiosId.env | cut -d ' ' -f 3 | cut -c 1-2)
BOARD_ID=$(grep '^BOARD_ID' $WORKSPACE/Conf/BiosId.env | cut -d ' ' -f 3 | cut -c 1-7)
BIOS_Name="$BOARD_ID"_"$Arch"_"$BUILD_TYPE"_"$VERSION_MAJOR"_"$VERSION_MINOR".ROM
BIOS_ID="$BOARD_ID"_"$Arch"_"$BUILD_TYPE"_"$VERSION_MAJOR"_"$VERSION_MINOR"_GCC.bin
cp -f $BUILD_PATH/FV/VLV.fd  $WORKSPACE/$BIOS_Name
SEC_VERSION=1.0.2.1060v5
cat $IFWI_HEADER_FILE $WORKSPACE/Vlv2Binaries/Vlv2MiscBinariesPkg/SEC/$SEC_VERSION/VLV_SEC_REGION.bin $WORKSPACE/Vlv2Binaries/Vlv2MiscBinariesPkg/SEC/$SEC_VERSION/Vacant.bin $WORKSPACE/$BIOS_Name > $WORKSPACE/edk2/$PLATFORM_PACKAGE/Stitch/$BIOS_ID
cp -f $BUILD_PATH/FV/VLV.fd $BUILD_PATH/FV/Vlv.ROM

echo Skip "Running BIOS_Signing ..."

echo
echo Build location:     $BUILD_PATH
echo BIOS ROM Created:   $BIOS_Name
echo
echo -------------------- The EDKII BIOS build has successfully completed. --------------------
echo

echo > $BUILD_PATH/FV/SYSTEMFIRMWAREUPDATECARGO.Fv
build -p $PLATFORM_PACKAGE/PlatformCapsuleGcc.dsc
