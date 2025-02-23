# Make sure that cache-sim is compiled
cd src
make release
cd ..
echo "Please specify the directory that contains the Spec benchmark traces."
read benchmark_dir

for file in $benchmark_dir/*.Z; do
    echo $file
    uncompress $file 
done


echo "Done with the decompression, now going to invoke cache-sim on each trace."

echo "Do you want to run with all associativity experiments, or just with default associativity? Enter 'yes' to run all experiments or nothing to do the default."
read all_input

echo "How many times do you want to run each trace?"
read num_iter

for file in $benchmark_dir/*; do
    for (( i = 0; i < $num_iter; i++ )); do
        if [[ "$all_input" == *"yes"* ]]; then  
            for assoc in 2 4 8; do
                echo "Running $file with assoc of $assoc."
                ./csim -f $file -a $assoc 
            done
        else 
            echo "Running $file with default assoc."
            ./csim -f $file
        fi
    done
done
