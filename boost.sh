curl https://dl.bintray.com/boostorg/release/1.66.0/source/boost_1_66_0.tar.gz > boost_1_66_0.tar.gz
tar -xzf boost_1_66_0.tar.gz
cd boost_1_66_0
./bootstrap.sh --prefix=/usr/local
./b2
./b2 install