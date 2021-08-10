#/bin/bash

cmd="/Users/cyarp/git/laminarCommCharacterize/plot/Docker/prod/execLaminarCommCharacterizePlotThisDir.sh"

files=$(ls)
origDir=$(pwd)

for file in ${files}
do
    fileFullPath="${origDir}/${file}"
    if [[ -d ${fileFullPath} ]]; then
        echo "cd ${fileFullPath}"
        cd "${fileFullPath}"
        echo "${cmd} --ylim 0 200 --title ${file}"
        ${cmd} --ylim 0 250 --title "${file}"
    fi
done

cd "${origDir}"