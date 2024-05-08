
# the first argument is the path to the test objec (i.e., new version of cobra) and the second argument is the path to the reference object (i.e., old version of cobra)
#  the third argument is a file containing the list of arguments handed over to thest and reference object
# all further arguments are masks of filenames passed to both the test and reference object
# the script compares the results printed to the commandline by the test and reference object

# check if the number of arguments is correct
if [ "$#" -lt 2 ]; then
    echo "Illegal number of parameters"
    exit 1
fi

# get the paths to the test and reference objects
test_obj=$1
ref_obj=$2

# get the masks of filenames
test_files="${@:4}"

result=0

echo $test_obj

# apply each test parameter set to each file mask to both the test and reference object
for file in $test_files; do
    # for param in $test_params; do
    while read -r param; do
        echo $param
        ct="$test_obj $param $file"
        echo $ct
        # get the results printed to the commandline by the test object
        test_result=$ct
        # echo $test_result

        cr="$ref_obj $param $file"
        echo $cr
        ref_result=$cr
        
        if [ "$test_result" != "$ref_result" ]; then
            echo "Test failed for file $file with parameters $param"
            echo "Test result: $test_result"
            echo "Reference result: $ref_result"
            $result=1
        fi
    done < $3
done

exit $result
