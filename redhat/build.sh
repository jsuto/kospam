#!/bin/bash

rm -rf master.tar.gz ~/rpmbuild; mkdir -p mkdir ~/rpmbuild/{SOURCES,BUILDROOT}
wget https://bitbucket.org/jsuto/clapf/get/master.tar.gz
tar zxf master.tar.gz
rm -rf clapf-0.5.1-rc1
mv jsuto-clapf-* clapf-0.5.1-rc1
tar cfz ~/rpmbuild/SOURCES/master.tar.gz clapf-0.5.1-rc1
(cd clapf-0.5.1-rc1; rpmbuild -bb piler.spec)

