cd src
tar xvf config4cpp.tar
cd config4cpp
sudo mkdir /usr/include/config4cpp
sudo cp config4cpp/include/config4cpp/* /usr/include/config4cpp
LD_LIBRARY_PATH=$LD_LIBRARY_PATH:`pwd`/lib
make clean
make
cd ..
make clean
make
mv ./NFDRScliV4 ../NFDRScliV4
