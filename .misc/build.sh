#!/bin/bash
# set -o pipefail
#

set -e

script_dir=$(cd "$(dirname "$0")" && pwd)
build_root=$(cd "${script_dir}/.." && pwd)
log_dir=$build_root
toolchainfile=" "
azureiotsdks=" "
build_folder=$build_root"/cmake/linux"
make=true


usage ()
{
    echo "build.sh [options]"
    echo "options"
    echo " -cl, --compileoption <value>  specify a compile option to be passed to gcc"
    echo "   Example: -cl -O1 -cl ..."
    echo " --toolchain-file <file>       pass cmake a toolchain file for cross compiling"
    echo " --azure-iot-sdk-c <directory>  azure iot sdks lib root directory"    
    exit 1
}

process_args ()
{
    save_next_arg=0
    extracloptions=" "

    for arg in $*
    do      
      if [ $save_next_arg == 1 ]
      then
        # save arg to pass to gcc
        extracloptions="$arg $extracloptions"
        save_next_arg=0
      elif [ $save_next_arg == 2 ]
      then
        # save arg to pass as toolchain file
        toolchainfile="$arg"
        save_next_arg=0
      elif [ $save_next_arg == 3 ]
      then
        #save azureiotsdsk lib path
        azureiotsdks="$arg"
        save_next_arg=0
      else
          case "$arg" in              
              "-cl" | "--compileoption" ) save_next_arg=1;;
              "--toolchain-file" ) save_next_arg=2;;
              "--azure-iot-sdk-c" ) save_next_arg=3;;
              * ) usage;;
          esac
      fi
    done

    if [ "$toolchainfile" != " " ]
    then
      toolchainfile=$(readlink -f $toolchainfile)
      toolchainfile="-DCMAKE_TOOLCHAIN_FILE=$toolchainfile"
    fi

    if [ "$azureiotsdks" != " " ]
    then
      azureiotsdks=$(readlink -f $azureiotsdks)
      azureiotsdks="-Dazure_IoT_Sdk_c=$azureiotsdks"
    fi
}

process_args $*

# setup yocto cross compile sysroot environments
envsetupfile=/opt/poky-edison/1.7.2/environment-setup-core2-32-poky-linux
if [ ! -f  "$envsetupfile"]; then
  echo "Error, pls make sure yocto cross compile toolchain installed successfully!"
  exit 1
fi
source $envsetupfile

rm -r -f $build_folder
mkdir -p $build_folder
pushd $build_folder
cmake $toolchainfile $azureiotsdks -DcompileOption_C:STRING="$extracloptions" $build_root

if [ "$make" = true ]
then
  # Set the default cores
  CORES=$(grep -c ^processor /proc/cpuinfo 2>/dev/null || sysctl -n hw.ncpu)
  
  # Make sure there is enough virtual memory on the device to handle more than one job  
  MINVSPACE="1500000"
  
  # Acquire total memory and total swap space setting them to zero in the event the command fails
  MEMAR=( $(sed -n -e 's/^MemTotal:[^0-9]*\([0-9][0-9]*\).*/\1/p' -e 's/^SwapTotal:[^0-9]*\([0-9][0-9]*\).*/\1/p' /proc/meminfo) )
  [ -z "${MEMAR[0]##*[!0-9]*}" ] && MEMAR[0]=0
  [ -z "${MEMAR[1]##*[!0-9]*}" ] && MEMAR[1]=0
  
  let VSPACE=${MEMAR[0]}+${MEMAR[1]}

  if [ "$VSPACE" -lt "$MINVSPACE" ] ; then
    CORES=1
  fi
  
  make --jobs=$CORES

fi

popd
