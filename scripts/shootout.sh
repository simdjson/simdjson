function report_perf() {
    algorithm=$1
    architecture=$2
    iterations=$3
    for run in {1..2}
    do
        echo -n "| $architecture | $algorithm "
        for file in "${@:4}"
        do
            ./parse -f $EXTRAARGS -n $iterations jsonexamples/$file.json | grep "stage 1 instructions" | sed -E "s/^.*stage 1 instructions:\s*([0-9]+)\s+cycles:\s*([0-9]+).+mis. branches:\s*([0-9]+).*/| \2 | \1 | \3 /" | tr -d "\n"
        done
        echo "|"
    done
}
echo -n "| Architecture | Algorithm "
for file in "${@:3}"
do
    echo -n "| $file Cycles | $file Instructions | $file Missed Branches "
done
echo "|"

git checkout jkeiser/lookup2_simpler_intel
make parse
report_perf lookup2 "$@"

git checkout jkeiser/lookup2
make parse
report_perf lookup2_old "$@"

git checkout master
make parse
report_perf fastvalidate "$@"
