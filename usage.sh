rm -rf pydmtools.egg-info dist
pip3 uninstall pydmtools

python3 setup.py sdist
cd dist
pip3 install pydmtools-0.1.1.tar.gz
