import sys, os

def resolve(arch):
    if sys.platform == 'darwin':
        os.environ['QT'] = '6.11.1'
    elif sys.platform == 'win32':
        print('Choosing Qt 6.')
        os.environ['QT'] = '6.11.1'
    return True
