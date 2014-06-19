from setuptools import setup

setup(name='${PACKAGE_NAME}',
    version='${PACKAGE_VERSION}',
    description='Ccnet is a framework for writing networked applications in C',
    url='${PACKAGE_URL}',
    author='Haiwen Inc.',
    author_email='freeplant@gmail.com',
    license='GPLv3',
    packages=['ccnet'],
    package_dir={ '': '${CMAKE_CURRENT_SOURCE_DIR}' },
    install_requires=['pysearpc'],
    zip_safe=False)
