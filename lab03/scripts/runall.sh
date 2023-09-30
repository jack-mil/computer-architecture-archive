
######################################################################################
# This scripts runs all four traces
# You will need to first compile your code in ../src before launching this script
# the results are stored in the ../results/ folder 
######################################################################################
directory="../results"
 
if [ ! -d "$directory" ]; then
    mkdir -p "$directory"
    echo "Directory '$directory' created."
else
    echo "Directory '$directory' already exists."
fi


########## ---------------  B1.1 ---------------- ################

../src/sim -pipewidth 1 -schedpolicy 0 ../traces/bzip2.ptr.gz > ../results/B1.1.bzip2.res
../src/sim -pipewidth 1 -schedpolicy 0 ../traces/gcc.ptr.gz > ../results/B1.1.gcc.res
../src/sim -pipewidth 1 -schedpolicy 0 ../traces/libq.ptr.gz > ../results/B1.1.libq.res
../src/sim -pipewidth 1 -schedpolicy 0 ../traces/mcf.ptr.gz > ../results/B1.1.mcf.res

########## ---------------  B1.2 ---------------- ################

../src/sim -pipewidth 1 -schedpolicy 1 ../traces/bzip2.ptr.gz > ../results/B1.2.bzip2.res
../src/sim -pipewidth 1 -schedpolicy 1 ../traces/gcc.ptr.gz > ../results/B1.2.gcc.res
../src/sim -pipewidth 1 -schedpolicy 1 ../traces/libq.ptr.gz > ../results/B1.2.libq.res
../src/sim -pipewidth 1 -schedpolicy 1 ../traces/mcf.ptr.gz > ../results/B1.2.mcf.res

########## ---------------  B2.1 ---------------- ################

../src/sim -pipewidth 1 -schedpolicy 0 -cache  ../traces/bzip2.ptr.gz > ../results/B2.1.bzip2.res
../src/sim -pipewidth 1 -schedpolicy 0 -cache ../traces/gcc.ptr.gz > ../results/B2.1.gcc.res
../src/sim -pipewidth 1 -schedpolicy 0 -cache  ../traces/libq.ptr.gz > ../results/B2.1.libq.res
../src/sim -pipewidth 1 -schedpolicy 0 -cache ../traces/mcf.ptr.gz > ../results/B2.1.mcf.res

########## ---------------  B2.2 ---------------- ################

../src/sim -pipewidth 1 -schedpolicy 1 -cache  ../traces/bzip2.ptr.gz > ../results/B2.2.bzip2.res
../src/sim -pipewidth 1 -schedpolicy 1 -cache ../traces/gcc.ptr.gz > ../results/B2.2.gcc.res
../src/sim -pipewidth 1 -schedpolicy 1 -cache  ../traces/libq.ptr.gz > ../results/B2.2.libq.res
../src/sim -pipewidth 1 -schedpolicy 1 -cache ../traces/mcf.ptr.gz > ../results/B2.2.mcf.res



########## ---------------  B3.1 ---------------- ################

../src/sim -pipewidth 1 -schedpolicy 0 -cache -exceptions ../traces/bzip2.ptr.gz > ../results/B3.1.bzip2.res
../src/sim -pipewidth 1 -schedpolicy 0 -cache -exceptions ../traces/gcc.ptr.gz > ../results/B3.1.gcc.res
../src/sim -pipewidth 1 -schedpolicy 0 -cache -exceptions ../traces/libq.ptr.gz > ../results/B3.1.libq.res
../src/sim -pipewidth 1 -schedpolicy 0 -cache -exceptions ../traces/mcf.ptr.gz > ../results/B3.1.mcf.res

########## ---------------  B3.2 ---------------- ################

../src/sim -pipewidth 1 -schedpolicy 1 -cache -exceptions ../traces/bzip2.ptr.gz > ../results/B3.2.bzip2.res
../src/sim -pipewidth 1 -schedpolicy 1 -cache -exceptions ../traces/gcc.ptr.gz > ../results/B3.2.gcc.res
../src/sim -pipewidth 1 -schedpolicy 1 -cache -exceptions ../traces/libq.ptr.gz > ../results/B3.2.libq.res
../src/sim -pipewidth 1 -schedpolicy 1 -cache -exceptions ../traces/mcf.ptr.gz > ../results/B3.2.mcf.res



########## ---------------  C.1 ---------------- ################

../src/sim -pipewidth 2 -schedpolicy 0 -cache ../traces/bzip2.ptr.gz > ../results/C1.bzip2.res
../src/sim -pipewidth 2 -schedpolicy 0 -cache ../traces/gcc.ptr.gz > ../results/C1.gcc.res
../src/sim -pipewidth 2 -schedpolicy 0 -cache ../traces/libq.ptr.gz > ../results/C1.libq.res
../src/sim -pipewidth 2 -schedpolicy 0 -cache ../traces/mcf.ptr.gz > ../results/C1.mcf.res

########## ---------------  C.2 ---------------- ################

../src/sim -pipewidth 2 -schedpolicy 1 -cache ../traces/bzip2.ptr.gz > ../results/C2.bzip2.res
../src/sim -pipewidth 2 -schedpolicy 1 -cache ../traces/gcc.ptr.gz > ../results/C2.gcc.res
../src/sim -pipewidth 2 -schedpolicy 1 -cache ../traces/libq.ptr.gz > ../results/C2.libq.res
../src/sim -pipewidth 2 -schedpolicy 1 -cache ../traces/mcf.ptr.gz > ../results/C2.mcf.res



########## ---------------  GenReport ---------------- ################

grep LAB3_CPI ../results/B?.*.res > report.txt
grep LAB3_CPI ../results/C?.*.res >> report.txt


######### ------- Goodbye -------- ##################

echo "Done. Check report.txt, and .res files in ../results";

