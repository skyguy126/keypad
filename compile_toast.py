from distutils.core import setup
import py2exe

setup(
    options = {'py2exe': {'optimize' : 2, 'bundle_files': 1, 'compressed': True, 'dll_excludes' : ['w9xpopen.exe']}},
    windows = [{'script': "toast.py"}],
    zipfile = None,
)
