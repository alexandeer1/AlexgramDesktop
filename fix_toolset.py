import os, sys

def fix_in_dir(directory):
    for root, dirs, files in os.walk(directory):
        for file in files:
            if file.endswith(('.vcxproj', '.sln')):
                path = os.path.join(root, file)
                try:
                    with open(path, 'r', encoding='utf-8') as f:
                        content = f.read()
                    if 'v143' in content:
                        new_content = content.replace('v143', 'v145')
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
