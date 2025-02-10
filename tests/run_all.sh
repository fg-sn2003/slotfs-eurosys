cd ./tests/Motivation-GC-1
./test.sh
cp Motivation-GC-1.pdf ../results
cd ../..

cd ./tests/Motivation-GC-2
./test.sh
cp Motivation-GC-2.pdf ../results
cd ../..

cd ./tests/FIO-ST
./test.sh
cp FIO-ST.pdf ../results
cd ../..

cd ./tests/FIO-MT
./test.sh
cp FIO-MT.pdf ../results
cd ../..

cd ./tests/Filebench
./test.sh
./test-madfs.sh
cp Filebench.pdf ../results
co performance-table-madfs ../results
cd ../..

