cd ./tests/Motivation-GC-1
./test.sh
cp Motivation-GC-1.pdf ../results
ipython -c "%run plot.ipynb"
cd ../..

cd ./tests/Motivation-GC-2
./test.sh
cp Motivation-GC-2.pdf ../results
ipython -c "%run plot.ipynb"
cd ../..

cd ./tests/FIO-ST
./test.sh
cp FIO-ST.pdf ../results
ipython -c "%run plot.ipynb"
cd ../..

cd ./tests/FIO-MT
./test.sh
cp FIO-MT.pdf ../results
ipython -c "%run plot.ipynb"
cd ../..

cd ./tests/Filebench
./test.sh
./test-madfs.sh
cp Filebench.pdf ../results
ipython -c "%run plot.ipynb"
co performance-table-madfs ../results
cd ../..

