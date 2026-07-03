# This file is part of Desktop App Toolkit,
# a set of libraries for developing nice desktop applications.
#
# For license and copyright information please follow this link:
# https://github.com/desktop-app/legal/blob/master/LEGAL

import sys, os, shutil, subprocess

def run(project, arguments, buildType=''):
    scriptPath = os.path.dirname(os.path.realpath(__file__))
    basePath = scriptPath + '/../out/' + buildType

    cmake = ['cmake']
    vsArch = ''
    explicitGenerator = False
    explicitToolset = ''
    for arg in arguments:
        if arg == 'debug':
            cmake.append('-DCMAKE_BUILD_TYPE=Debug')
        elif arg == 'x86' or arg == 'x64' or arg == 'arm':
            vsArch = arg
        elif arg != 'force':
            if arg.startswith('-G'):
                explicitGenerator = True
            elif arg.startswith('-T'):
                explicitToolset = arg
                continue
            cmake.append(arg)
    if sys.platform == 'win32' and not explicitGenerator:
        toolset = 'v143'
        vs_version = os.environ.get('VisualStudioVersion')
        if vs_version:
            major = vs_version.split('.')[0]
            if major == '18':
                toolset = 'v145'
            elif major == '17':
                toolset = 'v143'
            elif major == '16':
                toolset = 'v142'
        else:
            for path in [
                r"C:\Program Files (x86)\Microsoft Visual Studio\18",
                r"C:\Program Files\Microsoft Visual Studio\18"
            ]:
                if os.path.exists(path):
                    toolset = 'v145'
                    break
        toolset_arg = toolset
        if explicitToolset:
            val = explicitToolset[2:].strip()
            if val.startswith('host='):
                toolset_arg = f'{toolset},{val}' if toolset else val
            else:
                toolset_arg = val
        if vsArch == 'x64':
            cmake.append('-Ax64')
            if toolset_arg:
                cmake.append(f'-T{toolset_arg}')
        elif vsArch == 'arm':
            cmake.append('-AARM64')
            if explicitToolset:
                if toolset_arg:
                    cmake.append(f'-T{toolset_arg}')
        else:
            cmake.append('-AWin32') # default
            if toolset_arg:
                cmake.append(f'-T{toolset_arg}')
    elif vsArch != '':
        print("[ERROR] x86/x64/arm switch is supported only with Visual Studio.")
        return 1
    elif sys.platform == 'darwin':
        if not explicitGenerator:
            cmake.append('-GXcode')
    else:
        if not explicitGenerator:
            cmake.append('-GNinja Multi-Config')
        elif buildType:
            cmake.append('-DCMAKE_BUILD_TYPE=' + buildType)
        elif not '-DCMAKE_BUILD_TYPE=Debug' in cmake:
            cmake.append('-DCMAKE_BUILD_TYPE=Release')

    specialTarget = ''
    specialTargetFile = scriptPath + '/../' + project + '/build/target'
    if os.path.isfile(specialTargetFile):
        with open(specialTargetFile, 'r') as f:
            for line in f:
                target = line.strip()
                if len(target) > 0:
                    cmake.append('-DDESKTOP_APP_SPECIAL_TARGET=' + target)

    if sys.platform != 'win32':
        cmake.extend(['-Werror=dev', '-Werror=deprecated', '--warn-uninitialized'])
    cmake.extend(['..' if not buildType else '../..'])
    command = '"' + '" "'.join(cmake) + '"'

    if not os.path.exists(basePath):
        os.makedirs(basePath)
    elif 'force' in arguments:
        paths = os.listdir(basePath)
        for path in paths:
            low = path.lower();
            if not low.startswith('debug') and not low.startswith('release'):
                full = basePath + '/' + path
                if os.path.isdir(full):
                    shutil.rmtree(full, ignore_errors=False)
                else:
                    os.remove(full)
        print('Cleared previous.')
    os.chdir(basePath)
    return subprocess.call(command, shell=True)
