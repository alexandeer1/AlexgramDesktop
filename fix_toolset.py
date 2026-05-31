import os, sys

def get_target_toolset():
    vs_version = os.environ.get('VisualStudioVersion')
    if vs_version:
        major = vs_version.split('.')[0]
        if major == '18':
            return 'v145'
        elif major == '17':
            return 'v143'
        elif major == '16':
            return 'v142'
            
    # Check VS 2026/18 folders
    for path in [
        r"C:\Program Files (x86)\Microsoft Visual Studio\18",
        r"C:\Program Files\Microsoft Visual Studio\18"
    ]:
        if os.path.exists(path):
            return 'v145'
    return 'v143'

def fix_in_dir(directory):
    target = get_target_toolset()
    if target == 'v143':
        print("Target toolset is v143 (default). No fixing needed.")
        return
        
    print(f"Target toolset is {target}. Replacing v143 with {target}...")
    for root, dirs, files in os.walk(directory):
        for file in files:
            if file.endswith(('.vcxproj', '.sln')):
                path = os.path.join(root, file)
                try:
                    with open(path, 'r', encoding='utf-8') as f:
                        content = f.read()
                    if 'v143' in content:
                        new_content = content.replace('v143', target)
                        with open(path, 'w', encoding='utf-8') as f:
                            f.write(new_content)
                        print(f"Fixed toolset in {path}")
                except Exception as e:
                    print(f"Could not fix {path}: {e}")

if __name__ == "__main__":
    fix_in_dir('.')
    
    # Also scan and fix external libraries
    libs_dir = 'D:\\Libraries'
    if os.path.exists(libs_dir):
        print(f"Scanning and fixing toolsets in {libs_dir}...")
        fix_in_dir(libs_dir)
    else:
        print(f"Libraries directory not found at {libs_dir}")
