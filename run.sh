# Make sure that cache-sim is compiled
make release

echo "Please specify the directory that contains the Spec benchmark traces."
read benchmark_dir
mkdir benchmark_dir/uncompressed

for file in $benchmark_dir/*.Z; do
    echo $file
    uncompress $file 
done

for file in $benchmark_dir/*.din; do
    mv $file /uncompressed
done

echo "Done with the decompression, now going to invoke cache-sim on each trace."

for file in $benchmark_dir/uncompressed; do
    ./csim -f $file 
done
